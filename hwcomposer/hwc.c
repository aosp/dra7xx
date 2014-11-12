/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/resource.h>

#include <cutils/properties.h>
#include <cutils/log.h>
#include <cutils/native_handle.h>
#define HWC_REMOVE_DEPRECATED_VERSIONS 1
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware_legacy/uevent.h>
#include <system/graphics.h>
#include <utils/Timers.h>
#include <EGL/egl.h>
#include <linux/omapfb.h>

#include "hwc_dev.h"
#include "color_fmt.h"
#include "display.h"
#include "sw_vsync.h"
#include "utils.h"

static bool debug = false;

enum omap_plane {
 OMAP_DSS_GFX,
 OMAP_DSS_VIDEO1,
 OMAP_DSS_VIDEO2,
 OMAP_DSS_VIDEO3,
};

static void showfps(void)
{
    static int framecount = 0;
    static int lastframecount = 0;
    static nsecs_t lastfpstime = 0;
    static float fps = 0;
    char value[PROPERTY_VALUE_MAX];

    property_get("debug.hwc.showfps", value, "0");
    if (!atoi(value)) {
        return;
    }

    framecount++;
    if (!(framecount & 0x7)) {
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        nsecs_t diff = now - lastfpstime;
        fps = ((framecount - lastframecount) * (float)(s2ns(1))) / diff;
        lastfpstime = now;
        lastframecount = framecount;
        ALOGI("%d Frames, %f FPS", framecount, fps);
    }
}

static void reserve_overlays_for_displays(omap_hwc_device_t *hwc_dev)
{
    display_t *primary_display = hwc_dev->displays[HWC_DISPLAY_PRIMARY];
    uint32_t ovl_ix_base = OMAP_DSS_GFX;
    uint32_t max_overlays = MAX_DSS_OVERLAYS;
    uint32_t num_nonscaling_overlays = NUM_NONSCALING_OVERLAYS;

    /* If FB is not same resolution as LCD don't use GFX overlay. */
    if (primary_display->transform.scaling) {
        ovl_ix_base = OMAP_DSS_VIDEO1;
        max_overlays -= num_nonscaling_overlays;
        num_nonscaling_overlays = 0;
    }

    /*
     * We cannot atomically switch overlays from one display to another. First, they
     * have to be disabled, and the disabling has to take effect on the current display.
     * We keep track of the available number of overlays here.
     */
    uint32_t max_primary_overlays = max_overlays - hwc_dev->last_ext_ovls;
    uint32_t max_external_overlays = max_overlays - hwc_dev->last_int_ovls;

    composition_t *primary_comp = &primary_display->composition;

    primary_comp->tiler1d_slot_size = hwc_dev->tiler1d_slot_size;
    primary_comp->ovl_ix_base = ovl_ix_base;
    primary_comp->wanted_ovls = max_overlays;
    primary_comp->avail_ovls = max_primary_overlays;
    primary_comp->scaling_ovls = primary_comp->avail_ovls - num_nonscaling_overlays;
    primary_comp->used_ovls = 0;

    /*
     * For primary display we must reserve at least one overlay for FB, plus an extra
     * overlay for each protected layer.
     */
    layer_statistics_t *primary_layer_stats = &primary_display->layer_stats;
    uint32_t min_primary_overlays =
	    MIN(1 + primary_layer_stats->protected, max_overlays);

    /* Share available overlays between primary and external displays. */
    primary_comp->wanted_ovls = MAX(max_overlays / 2, min_primary_overlays);
    primary_comp->avail_ovls = MIN(max_primary_overlays, primary_comp->wanted_ovls);

}


/*
 * We're using "implicit" synchronization, so make sure we aren't passing any
 * sync object descriptors around.
 */
static void check_sync_fds_for_display(int disp, hwc_display_contents_1_t *list)
{
    if (disp < 0 || disp >= MAX_DISPLAYS || !list)
        return;

    if (list->retireFenceFd >= 0) {
        ALOGW("retireFenceFd[%u] was %d", disp, list->retireFenceFd);
        list->retireFenceFd = -1;
    }

    unsigned int j;
    for (j = 0; j < list->numHwLayers; j++) {
        hwc_layer_1_t* layer = &list->hwLayers[j];
        if (layer->acquireFenceFd >= 0) {
            ALOGW("acquireFenceFd[%u][%u] was %d, closing", disp, j, layer->acquireFenceFd);
            close(layer->acquireFenceFd);
            layer->acquireFenceFd = -1;
        }
        if (layer->releaseFenceFd >= 0) {
            ALOGW("releaseFenceFd[%u][%u] was %d", disp, j, layer->releaseFenceFd);
            layer->releaseFenceFd = -1;
        }
    }
}


static int hwc_prepare_for_display(omap_hwc_device_t *hwc_dev, int disp)
{
    if (!is_valid_display(hwc_dev, disp))
        return -ENODEV;

    if (!is_supported_display(hwc_dev, disp) || !is_active_display(hwc_dev, disp))
        return 0;

    display_t *display = hwc_dev->displays[disp];
    hwc_display_contents_1_t *list = display->contents;
    composition_t *comp = &display->composition;
    layer_statistics_t *layer_stats = &display->layer_stats;

    comp->num_buffers = 0;

    /*
     * The following priorities are used for different compositing HW:
     * 1 - BLITTER (policy = ALL)
     * 2 - DSS
     * 3 - BLITTER (policy = DEFAULT)
     * 4 - SGX
     */

    if (can_dss_render_all_layers(hwc_dev, disp)) {
        /* All layers can be handled by the DSS -- don't use SGX for composition */
        comp->use_sgx = false;
        comp->swap_rb = layer_stats->bgr != 0;
    } else {
        /* Use SGX for composition plus first 3 layers that are DSS renderable */
        comp->use_sgx = true;
        /* Only LCD can display BGR layers */
        comp->swap_rb = is_lcd_display(hwc_dev, disp) && 0/*is_bgr_format(display->disp_link->fb.format)*/;
    }

    /* setup DSS overlays */
    int z = 0;
    int fb_z = -1;
    bool scaled_gfx = false;
    uint32_t ovl_ix = comp->ovl_ix_base;
    uint32_t mem1d_used = 0;
    uint32_t i;

    /*
     * If the SGX is used or we are going to blit something we need a framebuffer and an overlay
     * for it. Reserve GFX for FB and begin using VID1 for DSS overlay layers.
     */
    bool needs_fb = comp->use_sgx;
    if (needs_fb) {
        hwc_dev->num_ovls++;
        ovl_ix++;
    }

    for (i = 0; i < list->numHwLayers; i++) {
        hwc_layer_1_t *layer = &list->hwLayers[i];

        if (hwc_dev->num_ovls < comp->avail_ovls && 0 &&
            can_dss_render_layer(hwc_dev, disp, layer) &&
            (!hwc_dev->force_sgx ||
            /* render protected layers via DSS */
            is_protected_layer(layer) ||
            is_upscaled_nv12_layer(hwc_dev, layer)) &&
            mem1d_used + get_required_mem1d_size(layer) <= comp->tiler1d_slot_size &&
            /* can't have a transparent overlay in the middle of the framebuffer stack */
            !(is_blended_layer(layer) && fb_z >= 0)) {

            /* render via DSS overlay */
            mem1d_used += get_required_mem1d_size(layer);
            layer->compositionType = HWC_OVERLAY;

            /*
             * This hint will not be used in vanilla ICS, but maybe in JellyBean,
             * it is useful to distinguish between blts and true overlays.
             */
            layer->hints |= HWC_HINT_TRIPLE_BUFFER;

            /* Clear FB above all opaque layers if rendering via SGX */
            if (comp->use_sgx && !is_blended_layer(layer))
                layer->hints |= HWC_HINT_CLEAR_FB;

            comp->buffers[comp->num_buffers] = layer->handle;

            adjust_drm_plane_to_layer(layer, z, &(comp->plane_info[hwc_dev->num_ovls]));

            /* Ensure GFX overlay is never scaled */
            if (ovl_ix == OMAP_DSS_GFX) {
                scaled_gfx = is_scaled_layer(layer) || is_nv12_layer(layer);
            } else if (scaled_gfx && !is_scaled_layer(layer) && !is_nv12_layer(layer)) {
                /* Swap GFX overlay with this one. If GFX is used it's always at index 0. */
                comp->planes[hwc_dev->num_ovls].plane_id = comp->planes[0].plane_id;
                comp->planes[0].plane_id = ovl_ix;
                scaled_gfx = false;
            }

            hwc_dev->num_ovls++;
            comp->num_buffers++;
            ovl_ix++;
            z++;
        } else if (comp->use_sgx) {
            if (fb_z < 0) {
                /* NOTE: we are not handling transparent cutout for now */
                fb_z = z;
                z++;
            } else {
                /* move fb z-order up (by lowering dss layers) */
                while (fb_z < z - 1) {
                    //TBD: set zorder //comp->planes[1 + fb_z++].cfg.zorder--;
                }
            }
        }
    }

    /* If scaling GFX (e.g. only 1 scaled surface) use a VID overlay */
    if (scaled_gfx)
        comp->planes[0].plane_id = (ovl_ix < comp->avail_ovls) ? ovl_ix : (MAX_DSS_OVERLAYS - 1);

    /* If the SGX is not used and there is blit data we need a framebuffer and
     * a DSS pipe well configured for it
     */
    if (needs_fb) {
        /* assign a z-layer for fb */
        if (fb_z < 0) {
            if (layer_stats->count)
                ALOGW("Should have assigned z-layer for fb");
            fb_z = z++;
        }

        //TBD: setup_framebuffer(hwc_dev, disp, comp->ovl_ix_base, fb_z);
    }

    comp->used_ovls = hwc_dev->num_ovls;
    if (disp == HWC_DISPLAY_PRIMARY)
        hwc_dev->last_int_ovls = comp->used_ovls;
    else
        hwc_dev->last_ext_ovls = comp->used_ovls;

    //TBD: setup_dsscomp_manager(hwc_dev, disp);


    /* If display is blanked or not configured drop compositions */
    if (display->blanked)
      hwc_dev->num_ovls = 0;

    return 0;
}

static int hwc_prepare(struct hwc_composer_device_1 *dev, size_t numDisplays,
        hwc_display_contents_1_t** displays)
{
    if (!numDisplays || displays == NULL) {
        return 0;
    }

    omap_hwc_device_t *hwc_dev = (omap_hwc_device_t *)dev;
    int err = 0;
    int disp_err;
    uint32_t i;

    pthread_mutex_lock(&hwc_dev->ctx_mutex);

    set_display_contents(hwc_dev, numDisplays, displays);

    for (i = 0; i < numDisplays; i++) {
        if (is_active_display(hwc_dev, i)) {
            display_t *display = hwc_dev->displays[i];
            hwc_display_contents_1_t *contents;

            if (display->update_transform) {
                disp_err = setup_display_tranfsorm(hwc_dev, i);
                if (!err && disp_err)
                    err = disp_err;
            }

            contents = display->contents;

            gather_layer_statistics(hwc_dev, i, contents);
        }
    }

    reserve_overlays_for_displays(hwc_dev);

    for (i = 0; i < numDisplays; i++) {
        if (displays[i]) {
            disp_err = hwc_prepare_for_display(hwc_dev, i);
            if (!err && disp_err)
                err = disp_err;
        }
    }

    /*
     * The display transform application has to be separated from prepare() loop so that in case
     * of mirroring we clone original overlay configuration. Otherwise cloned overlays will have
     * both primary and external display transform applied, which is not intended.
     */
    for (i = 0; i < numDisplays; i++) {
        if (is_active_display(hwc_dev, i)) {
            disp_err = apply_display_transform(hwc_dev, i);
            if (!err && disp_err)
                err = disp_err;

            disp_err = validate_display_composition(hwc_dev, i);
            if (!err && disp_err)
                err = disp_err;
        }
    }

    pthread_mutex_unlock(&hwc_dev->ctx_mutex);

    return err;
}

static int hwc_set_for_display(omap_hwc_device_t *hwc_dev, int disp, hwc_display_contents_1_t *list,
        bool *invalidate)
{
    if (!is_valid_display(hwc_dev, disp))
        return list ? -ENODEV : 0;

    if (!is_supported_display(hwc_dev, disp))
        return 0;

    display_t *display = hwc_dev->displays[disp];
    layer_statistics_t *layer_stats = &display->layer_stats;
    composition_t *comp = &display->composition;

    if (disp != HWC_DISPLAY_PRIMARY) {
        if (comp->wanted_ovls && (comp->avail_ovls < comp->wanted_ovls) &&
                (layer_stats->protected || !comp->avail_ovls))
            *invalidate = true;
    }

    hwc_display_t dpy = NULL;
    hwc_surface_t sur = NULL;
    if (list != NULL) {
        dpy = list->dpy;
        sur = list->sur;
    }

 /* Blanking primary display is necessary if the bootloader can't be trusted
    * However, this causes issues with early camera use-case. The latest
    * bootloader seems to have the same configuration what hwc expects
    * so this config could be safely disabled
    */
#ifdef BLANK_PRIMARY_DISPLAY
    static bool first_set = true;
    if (first_set) {
        reset_primary_display(hwc_dev);
        first_set = false;
    }
#endif

    int err = 0;

    /* The list can be NULL which means HWC is temporarily disabled. However, if dpy and sur
     * are NULL it means we're turning the screen off.
     */
    if (dpy && sur) {
        if (comp->use_sgx) {
            buffer_handle_t framebuffer = NULL;
            if (layer_stats->framebuffer) {
                /* Layer with HWC_FRAMEBUFFER_TARGET should be last in the list. The buffer handle
                 * is updated by SurfaceFlinger after prepare() call, so FB slot has to be updated
                 * in set().
                 */
                framebuffer = list->hwLayers[list->numHwLayers - 1].handle;
            }

            if (framebuffer) {
                comp->buffers[comp->planes[0].fb_id] = framebuffer;
            } else {
                ALOGE("set[%d]: No buffer is provided for GL composition", disp);
                return -EFAULT;
            }
        }

        update_display(hwc_dev, disp, list);

        if (disp == HWC_DISPLAY_PRIMARY)
            showfps();
     }

     check_sync_fds_for_display(disp, list);

    return err;
}


static int hwc_set(struct hwc_composer_device_1 *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    if (!numDisplays || displays == NULL) {
        ALOGD("set: empty display list");
        return 0;
    }

    omap_hwc_device_t *hwc_dev = (omap_hwc_device_t*)dev;

    pthread_mutex_lock(&hwc_dev->ctx_mutex);

    bool invalidate = false;
    uint32_t i;
    int err = 0;

    for (i = 0; i < numDisplays; i++) {
        int disp_err = hwc_set_for_display(hwc_dev, i, displays[i], &invalidate);
        if (!err && disp_err)
            err = disp_err;
    }

    if (hwc_dev->force_sgx > 0)
        hwc_dev->force_sgx--;

    pthread_mutex_unlock(&hwc_dev->ctx_mutex);

    if (invalidate && hwc_dev->cb_procs && hwc_dev->cb_procs->invalidate)
        hwc_dev->cb_procs->invalidate(hwc_dev->cb_procs);

    return err;
}


static int hwc_device_close(hw_device_t* device)
{
    omap_hwc_device_t *hwc_dev = (omap_hwc_device_t *) device;;

    if (hwc_dev) {
        /* pthread will get killed when parent process exits */
        pthread_mutex_destroy(&hwc_dev->ctx_mutex);
        free_displays(hwc_dev);
        free(hwc_dev);
    }

    return 0;
}

static void hwc_registerProcs(struct hwc_composer_device_1* dev,
                                    hwc_procs_t const* procs)
{
    omap_hwc_device_t *hwc_dev = (omap_hwc_device_t *) dev;

    hwc_dev->cb_procs = (typeof(hwc_dev->cb_procs)) procs;
}

static int hwc_query(struct hwc_composer_device_1* dev, int what, int* value)
{
    omap_hwc_device_t *hwc_dev = (omap_hwc_device_t *) dev;

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // we don't support the background layer yet
        value[0] = 0;
        break;
    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = 1000000000.0 / hwc_dev->displays[0]->disp_link.mode->vrefresh;
        break;
    default:
        // unsupported query
        return -EINVAL;
    }
    return 0;
}

static int hwc_eventControl(struct hwc_composer_device_1* dev,
        int dpy, int event, int enabled)
{
    omap_hwc_device_t *hwc_dev = (omap_hwc_device_t *) dev;


    switch (event) {
    case HWC_EVENT_VSYNC:
    {
        int val = !!enabled;
        int err;

        primary_display_t *primary = get_primary_display_info(hwc_dev);
        if (!primary)
            return -ENODEV;

        if (primary->use_sw_vsync) {
            if (enabled)
                start_sw_vsync(hwc_dev);
            else
                stop_sw_vsync();
            return 0;
        }

        return 0;
    }
    default:
        return -EINVAL;
    }
}

static int hwc_blank(struct hwc_composer_device_1 *dev, int disp, int blank)
{
    omap_hwc_device_t *hwc_dev = (omap_hwc_device_t *) dev;

    pthread_mutex_lock(&hwc_dev->ctx_mutex);

    if (!is_valid_display(hwc_dev, disp)) {
        pthread_mutex_unlock(&hwc_dev->ctx_mutex);
        return -ENODEV;
    }

    /*
     * We're using an older method of screen blanking based on early_suspend in the kernel.
     * No need to do anything here except updating the display state.
     */
    display_t *display = hwc_dev->displays[disp];
    int err = 0;

    display->blanked = blank;


    pthread_mutex_unlock(&hwc_dev->ctx_mutex);

    return err;
}

static int hwc_getDisplayConfigs(struct hwc_composer_device_1* dev, int disp, uint32_t* configs, size_t* numConfigs)
{
    return get_display_configs((omap_hwc_device_t *)dev, disp, configs, numConfigs);
}

static int hwc_getDisplayAttributes(struct hwc_composer_device_1* dev, int disp,
                                    uint32_t config, const uint32_t* attributes, int32_t* values)
{
    return get_display_attributes((omap_hwc_device_t *)dev, disp, config, attributes, values);
}


static void hwc_dump(struct hwc_composer_device_1 *dev, char *buff, int buff_len)
{

}

static int hwc_device_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
    omap_hwc_module_t *hwc_mod = (omap_hwc_module_t *)module;
    omap_hwc_device_t *hwc_dev;
    int err = 0, i;

    if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
        return -EINVAL;
    }

    hwc_dev = (omap_hwc_device_t *)malloc(sizeof(*hwc_dev));
    if (hwc_dev == NULL)
        return -ENOMEM;

    memset(hwc_dev, 0, sizeof(*hwc_dev));

    hwc_dev->device.common.tag = HARDWARE_DEVICE_TAG;
    hwc_dev->device.common.version = HWC_DEVICE_API_VERSION_1_1;

    hwc_dev->device.common.module = (hw_module_t *)module;
    hwc_dev->device.common.close = hwc_device_close;
    hwc_dev->device.prepare = hwc_prepare;
    hwc_dev->device.set = hwc_set;
    hwc_dev->device.eventControl = hwc_eventControl;
    hwc_dev->device.blank = hwc_blank;
    hwc_dev->device.dump = hwc_dump;
    hwc_dev->device.registerProcs = hwc_registerProcs;
    hwc_dev->device.getDisplayConfigs = hwc_getDisplayConfigs;
    hwc_dev->device.getDisplayAttributes = hwc_getDisplayAttributes;
    hwc_dev->device.query = hwc_query;

    *device = &hwc_dev->device.common;

    err = init_primary_display(hwc_dev);
    if (err)
        goto done;

    if (pthread_mutex_init(&hwc_dev->ctx_mutex, NULL)) {
        ALOGE("failed to create mutex (%d): %m", errno);
        err = -errno;
        goto done;
    }

    /* get debug properties */
    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.rgb_order", value, "1");
    hwc_dev->flags_rgb_order = atoi(value);
    property_get("persist.hwc.nv12_only", value, "0");
    hwc_dev->flags_nv12_only = atoi(value);
    property_get("debug.hwc.idle", value, "250");
    hwc_dev->idle = atoi(value);

    ALOGI("open_device(rgb_order=%d nv12_only=%d)",
        hwc_dev->flags_rgb_order, hwc_dev->flags_nv12_only);

    property_get("persist.hwc.upscaled_nv12_limit", value, "2.");
    sscanf(value, "%f", &hwc_dev->upscaled_nv12_limit);
    if (hwc_dev->upscaled_nv12_limit < 0. || hwc_dev->upscaled_nv12_limit > 2048.) {
        ALOGW("Invalid upscaled_nv12_limit (%s), setting to 2.", value);
        hwc_dev->upscaled_nv12_limit = 2.;
    }

    hwc_dev->tiler1d_slot_size = 32*1024*1024; //32MB    

done:
    if (err && hwc_dev) 
       hwc_device_close((hw_device_t*)hwc_dev);

    return err;
}

static struct hw_module_methods_t module_methods = {
    .open = hwc_device_open,
};

omap_hwc_module_t HAL_MODULE_INFO_SYM = {
    .base = {
        .common = {
            .tag =                  HARDWARE_MODULE_TAG,
            .module_api_version =   HWC_MODULE_API_VERSION_0_1,
            .hal_api_version =      HARDWARE_HAL_API_VERSION,
            .id =                   HWC_HARDWARE_MODULE_ID,
            .name =                 "Jacinto6 Hardware Composer HAL",
            .author =               "Texas Instruments",
            .methods =              &module_methods,
        },
    },
};
