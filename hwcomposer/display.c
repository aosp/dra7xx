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
#include <stdint.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#include <linux/fb.h>

#include "hwc_dev.h"
#include "color_fmt.h"
#include "display.h"
#include "layer.h"
#include "sw_vsync.h"
#include "utils.h"

#define LCD_DISPLAY_CONFIGS 1
#define LCD_DISPLAY_FPS 60
#define LCD_DISPLAY_DEFAULT_DPI 150

/* Currently SF cannot handle more than 1 config */
#define HDMI_DISPLAY_CONFIGS 1
#define HDMI_DISPLAY_FPS 60
#define HDMI_DISPLAY_DEFAULT_DPI 75

#define MAX_DISPLAY_ID (MAX_DISPLAYS - 1)
#define INCH_TO_MM 25.4f

/* Used by property settings */
enum {
    EXT_ROTATION    = 3,        /* rotation while mirroring */
    EXT_HFLIP       = (1 << 2), /* flip l-r on output (after rotation) */
};


static int crop_plane_to_rect(struct hwc_rect vis_rect, struct drm_mode_set_plane *plane)
{
    struct {
        int xy[2];
        int wh[2];
    } crop, win;
    struct {
        int lt[2];
        int rb[2];
    } vis;
    win.xy[0] = plane->crtc_x; win.xy[1] = plane->crtc_y;
    win.wh[0] = plane->crtc_w; win.wh[1] = plane->crtc_h;
    crop.xy[0] = plane->src_x; crop.xy[1] = plane->src_y;
    crop.wh[0] = plane->src_w; crop.wh[1] = plane->src_h;
    vis.lt[0] = vis_rect.left; vis.lt[1] = vis_rect.top;
    vis.rb[0] = vis_rect.right; vis.rb[1] = vis_rect.bottom;

    int c;
    bool swap = 0;//TBD: Add rotation support later

    /* align crop window with display coordinates */
    if (swap)
        crop.xy[1] -= (crop.wh[1] = -crop.wh[1]);

    for (c = 0; c < 2; c++) {
        /* see if complete buffer is outside the vis or it is
          fully cropped or scaled to 0 */
        if (win.wh[c] <= 0 || vis.rb[c] <= vis.lt[c] ||
            win.xy[c] + win.wh[c] <= vis.lt[c] ||
            win.xy[c] >= vis.rb[c] ||
            !crop.wh[c ^ swap])
            return -ENOENT;

        /* crop left/top */
        if (win.xy[c] < vis.lt[c]) {
            /* correction term */
            int a = (vis.lt[c] - win.xy[c]) * crop.wh[c ^ swap] / win.wh[c];
            crop.xy[c ^ swap] += a;
            crop.wh[c ^ swap] -= a;
            win.wh[c] -= vis.lt[c] - win.xy[c];
            win.xy[c] = vis.lt[c];
        }
        /* crop right/bottom */
        if (win.xy[c] + win.wh[c] > vis.rb[c]) {
            crop.wh[c ^ swap] = crop.wh[c ^ swap] * (vis.rb[c] - win.xy[c]) / win.wh[c];
            win.wh[c] = vis.rb[c] - win.xy[c];
        }

        if (!crop.wh[c ^ swap] || !win.wh[c])
            return -ENOENT;
    }

    /* realign crop window to buffer coordinates */
    if (swap)
        crop.xy[1] -= (crop.wh[1] = -crop.wh[1]);

    plane->crtc_x = win.xy[0]; plane->crtc_y = win.xy[1];
    plane->crtc_w = win.wh[0]; plane->crtc_h = win.wh[1];
    plane->src_x = crop.xy[0]; plane->src_y = crop.xy[1];
    plane->src_w = crop.wh[0]; plane->src_h = crop.wh[1];

    return 0;
}

void adjust_drm_plane_to_layer(hwc_layer_1_t const *layer, int zorder, struct drm_mode_set_plane *plane)
{
    IMG_native_handle_t *handle = (IMG_native_handle_t *)layer->handle;

//    setup_drm_plane(handle->iWidth, handle->iHeight, handle->iFormat, is_blended_layer(layer), zorder, ovl);

    /* convert transformation - assuming 0-set config */
    if (layer->transform & HWC_TRANSFORM_FLIP_H) {
    }

    if (layer->transform & HWC_TRANSFORM_FLIP_V) {

    }

    if (layer->transform & HWC_TRANSFORM_ROT_90) {

    }

    /* display position */
    plane->crtc_x = layer->displayFrame.left;
    plane->crtc_y = layer->displayFrame.top;
    plane->crtc_w = WIDTH(layer->displayFrame);
    plane->crtc_h = HEIGHT(layer->displayFrame);

    /* crop */
    plane->crtc_x = layer->sourceCrop.left;
    plane->crtc_y = layer->sourceCrop.top;
    plane->crtc_w = WIDTH(layer->sourceCrop);
    plane->crtc_h = HEIGHT(layer->sourceCrop);
}

void adjust_drm_plane_to_display(omap_hwc_device_t *hwc_dev, int disp, struct drm_mode_set_plane *plane)
{
    display_t *display = hwc_dev->displays[disp];
    if (!display)
        return;

    if (crop_plane_to_rect(display->transform.region, plane) != 0) {
        return;
    }

    hwc_rect_t win = {plane->crtc_x, plane->crtc_y, plane->crtc_x + plane->crtc_w, plane->crtc_y + plane->crtc_h};

    transform_rect(display->transform.matrix, &win);

    plane->crtc_x = win.left;
    plane->crtc_y = win.top;
    plane->crtc_w = WIDTH(win);
    plane->crtc_h = HEIGHT(win);

}

int validate_drm_composition(omap_hwc_device_t *hwc_dev, display_t *display)
{
//TBD
return true;
}


void setup_drm_plane(int width, int height, uint32_t format, bool blended, int zorder, drmModePlane *plane)
{

}


bool can_dss_scale(omap_hwc_device_t *hwc_dev, uint32_t src_w, uint32_t src_h,
                   uint32_t dst_w, uint32_t dst_h, bool is_2d,
                   dss_platform_info_t *limits, uint32_t pclk)
{
    uint32_t fclk = limits->fclk / 1000;
    uint32_t min_src_w = DIV_ROUND_UP(src_w, is_2d ? limits->max_xdecim_2d : limits->max_xdecim_1d);
    uint32_t min_src_h = DIV_ROUND_UP(src_h, is_2d ? limits->max_ydecim_2d : limits->max_ydecim_1d);

    /* NOTE: no support for checking YUV422 layers that are tricky to scale */

    /* FIXME: limit vertical downscale well below theoretical limit as we saw display artifacts */
    if (dst_h < src_h / 4)
        return false;

    /* max downscale */
    if (dst_h * limits->max_downscale < min_src_h)
        return false;

    /* for manual panels pclk is 0, and there are no pclk based scaling limits */
    if (!pclk)
        return !(dst_w < src_w / limits->max_downscale / (is_2d ? limits->max_xdecim_2d : limits->max_xdecim_1d));

    /* :HACK: limit horizontal downscale well below theoretical limit as we saw display artifacts */
    if (dst_w * 4 < src_w)
        return false;

    /* max horizontal downscale is 4, or the fclk/pixclk */
    if (fclk > pclk * limits->max_downscale)
        fclk = pclk * limits->max_downscale;

    /* for small parts, we need to use integer fclk/pixclk */
    if (src_w < limits->integer_scale_ratio_limit)
        fclk = fclk / pclk * pclk;

    if ((uint32_t) dst_w * fclk < min_src_w * pclk)
        return false;

    return true;
}

bool can_dss_render_all_layers(omap_hwc_device_t *hwc_dev, int disp)
{
    display_t *display = hwc_dev->displays[disp];
    layer_statistics_t *layer_stats = &display->layer_stats;
    composition_t *comp = &display->composition;
    bool support_bgr = is_lcd_display(hwc_dev, disp);
    bool tform = false;

    return  !hwc_dev->force_sgx &&
            /* Must have at least one layer if using composition bypass to get sync object */
            layer_stats->composable &&
            layer_stats->composable <= comp->avail_ovls &&
            layer_stats->composable == layer_stats->count &&
            layer_stats->scaled <= comp->scaling_ovls &&
            layer_stats->nv12 <= comp->scaling_ovls &&
            /* Fits into TILER slot */
            layer_stats->mem1d_total <= comp->tiler1d_slot_size &&
            /* We cannot clone non-NV12 transformed layers */
            (!tform || (layer_stats->nv12 == layer_stats->composable)) &&
            /* Only LCD can display BGR */
            (layer_stats->bgr == 0 || (layer_stats->rgb == 0 && support_bgr) || !hwc_dev->flags_rgb_order) &&
            /* If nv12_only flag is set DSS should only render NV12 */
            (!hwc_dev->flags_nv12_only || (layer_stats->bgr == 0 && layer_stats->rgb == 0));
}

bool can_dss_render_layer(omap_hwc_device_t *hwc_dev, int disp, hwc_layer_1_t *layer)
{
    display_t *display = hwc_dev->displays[disp];
    composition_t *comp = &display->composition;
    bool tform = false;

    return is_composable_layer(hwc_dev, disp, layer) &&
           /* Cannot rotate non-NV12 layers on external display */
           (!tform || is_nv12_layer(layer)) &&
           /* Skip non-NV12 layers if also using SGX (if nv12_only flag is set) */
           (!hwc_dev->flags_nv12_only || (!comp->use_sgx || is_nv12_layer(layer))) &&
           /* Make sure RGB ordering is consistent (if rgb_order flag is set) */
           (!(comp->swap_rb ? is_rgb_layer(layer) : is_bgr_layer(layer)) || !hwc_dev->flags_rgb_order);
}


static void free_display(display_t *display)
{
    if (display) {
        if (display->configs)
            free(display->configs);
        if (display->composition.buffers)
            free(display->composition.buffers);

        free(display);
    }
}

static void remove_display(omap_hwc_device_t *hwc_dev, int disp)
{
    free_display(hwc_dev->displays[disp]);
    hwc_dev->displays[disp] = NULL;
}

static int allocate_display(size_t display_data_size, uint32_t max_configs, display_t **new_display)
{
    int err = 0;

    display_t *display = (display_t *)malloc(display_data_size);
    if (display == NULL) {
        err = -ENOMEM;
        goto err_out;
    }

    memset(display, 0, display_data_size);

    display->num_configs = max_configs;
    size_t config_data_size = sizeof(*display->configs) * display->num_configs;
    display->configs = (display_config_t *)malloc(config_data_size);
    if (display->configs == NULL) {
        err = -ENOMEM;
        goto err_out;
    }

    memset(display->configs, 0, config_data_size);

    /* Allocate the maximum buffers that we can receive from HWC */
    display->composition.buffers = malloc(sizeof(buffer_handle_t) * MAX_COMPOSITION_BUFFERS);
    if (!display->composition.buffers) {
        err = -ENOMEM;
        goto err_out;
    }

    memset(display->composition.buffers, 0, sizeof(buffer_handle_t) * MAX_COMPOSITION_BUFFERS);

err_out:

    if (err) {
        ALOGE("Failed to allocate display (configs = %d)", max_configs);
        free_display(display);
    } else {
        *new_display = display;
    }

    return err;
}


static void setup_config(display_config_t *config, int xres, int yres, struct dsscomp_display_info *info,
                         int default_fps, int default_dpi)
{
    config->xres = xres;
    config->yres = yres;
    config->fps = default_fps;

   {
        config->xdpi = default_dpi;
        config->ydpi = default_dpi;
    }
}

static void setup_lcd_config(display_config_t *config, int xres, int yres, struct dsscomp_display_info *info)
{
    setup_config(config, xres, yres, info, LCD_DISPLAY_FPS, LCD_DISPLAY_DEFAULT_DPI);
}

static void setup_hdmi_config(display_config_t *config, int xres, int yres, struct dsscomp_display_info *info)
{
    setup_config(config, xres, yres, info, HDMI_DISPLAY_FPS, HDMI_DISPLAY_DEFAULT_DPI);
}

static int init_primary_lcd_display(omap_hwc_device_t *hwc_dev, uint32_t xres, uint32_t yres, struct dsscomp_display_info *info)
{
    int err;

    err = allocate_display(sizeof(primary_lcd_display_t), LCD_DISPLAY_CONFIGS, &hwc_dev->displays[HWC_DISPLAY_PRIMARY]);
    if (err)
        return err;

    display_t *display = hwc_dev->displays[HWC_DISPLAY_PRIMARY];

    setup_lcd_config(&display->configs[0], xres, yres, info);

    display->type = DISP_TYPE_LCD;

    return 0;
}

static void update_primary_display_orientation(omap_hwc_device_t *hwc_dev) {
    display_t *display = hwc_dev->displays[HWC_DISPLAY_PRIMARY];

    primary_display_t *primary = get_primary_display_info(hwc_dev);
    if (!primary)
        return;

}

static void set_primary_display_transform_matrix(omap_hwc_device_t *hwc_dev)
{
    display_t *display = hwc_dev->displays[HWC_DISPLAY_PRIMARY];

    /* Create primary display translation matrix */
    int lcd_w = display->disp_link.mode->hdisplay;
    int lcd_h = display->disp_link.mode->vdisplay;
    int orig_w = display->disp_link.fb.width;
    int orig_h = display->disp_link.fb.height;
    hwc_rect_t region = {.left = 0, .top = 0, .right = orig_w, .bottom = orig_h};
    display_transform_t *transform = &display->transform;


    transform->region = region;
    transform->rotation = ((lcd_w > lcd_h) ^ (orig_w > orig_h)) ? 1 : 0;
    transform->scaling = ((lcd_w != orig_w) || (lcd_h != orig_h));

    ALOGI("Transforming FB (%dx%d) => (%dx%d) rot%d", orig_w, orig_h, lcd_w, lcd_h, transform->rotation);

    /* Reorientation matrix is:
       m = (center-from-target-center) * (scale-to-target) * (mirror) * (rotate) * (center-to-original-center) */
    memcpy(transform->matrix, unit_matrix, sizeof(unit_matrix));
    translate_matrix(transform->matrix, -(orig_w >> 1), -(orig_h >> 1));
    rotate_matrix(transform->matrix, transform->rotation);

    if (transform->rotation & 1)
         SWAP(orig_w, orig_h);

    scale_matrix(transform->matrix, orig_w, lcd_w, orig_h, lcd_h);
    translate_matrix(transform->matrix, lcd_w >> 1, lcd_h >> 1);
}

static void set_external_display_transform_matrix(omap_hwc_device_t *hwc_dev, int disp)
{
    display_t *display = hwc_dev->displays[disp];
    display_transform_t *transform = &display->transform;
    int orig_xres = WIDTH(transform->region);
    int orig_yres = HEIGHT(transform->region);
    float orig_center_x = transform->region.left + orig_xres / 2.0f;
    float orig_center_y = transform->region.top + orig_yres / 2.0f;

    /* Reorientation matrix is:
       m = (center-from-target-center) * (scale-to-target) * (mirror) * (rotate) * (center-to-original-center) */

    memcpy(transform->matrix, unit_matrix, sizeof(unit_matrix));
    translate_matrix(transform->matrix, -orig_center_x, -orig_center_y);
    rotate_matrix(transform->matrix, transform->rotation);
    if (transform->hflip)
        scale_matrix(transform->matrix, 1, -1, 1, 1);

    primary_display_t *primary = get_primary_display_info(hwc_dev);
    if (!primary)
        return;

    float xpy = primary->xpy;

    if (transform->rotation & 1) {
        SWAP(orig_xres, orig_yres);
        xpy = 1.0f / xpy;
    }

    /* get target size */
    uint32_t adj_xres, adj_yres;
    uint32_t width, height;
    int xres, yres;

    if (is_hdmi_display(hwc_dev, disp)) {
        hdmi_display_t *hdmi = &((external_hdmi_display_t*)display)->hdmi;
        width = hdmi->width;
        height = hdmi->height;
        xres = display->disp_link.mode->hdisplay;;
        yres = display->disp_link.mode->hdisplay;;
    } else {
        display_config_t *config = &display->configs[display->active_config_ix];
        width = 0;
        height = 0;
        xres = config->xres;
        yres = config->yres;
    }

    display->transform.scaling = ((xres != orig_xres) || (yres != orig_yres));

    get_max_dimensions(orig_xres, orig_yres, xpy,
                       xres, yres, width, height,
                       &adj_xres, &adj_yres);

    scale_matrix(transform->matrix, orig_xres, adj_xres, orig_yres, adj_yres);
    translate_matrix(transform->matrix, xres >> 1, yres >> 1);
}

static int setup_external_display_transform(omap_hwc_device_t *hwc_dev, int disp)
{
    display_t *display = hwc_dev->displays[disp];

    if (display->opmode == DISP_MODE_PRESENTATION) {
        display_config_t *config = &display->configs[display->active_config_ix];
        struct hwc_rect src_region = { .right = config->xres, .bottom = config->yres };
        display->transform.region = src_region;
    } else {
        primary_display_t *primary = get_primary_display_info(hwc_dev);
        if (!primary)
            return -ENODEV;

        display->transform.region = primary->mirroring_region;
    }

    uint32_t xres = WIDTH(display->transform.region);
    uint32_t yres = HEIGHT(display->transform.region);

    if (!(xres && yres))
        return -EINVAL;

    int rot_flip = (yres > xres) ? 3 : 0;
    display->transform.rotation = rot_flip & EXT_ROTATION;
    display->transform.hflip = (rot_flip & EXT_HFLIP) > 0;

    if (display->transform.rotation & 1)
        SWAP(xres, yres);

    set_external_display_transform_matrix(hwc_dev, disp);

    return 0;
}

static int init_hdmi_display(omap_hwc_device_t *hwc_dev, int disp)
{
    hdmi_display_t *hdmi = (hdmi_display_t*)hwc_dev->displays[disp];
    if (!hdmi)
        return -ENODEV;

    return 0;
}

static void vblank_handler(int fd, unsigned int frame, unsigned int sec,
        unsigned int usec, void *data)
{
    kms_display_t *kdisp = (kms_display_t *)data;
    const hwc_procs_t *procs = kdisp->ctx->cb_procs;

    if (kdisp->vsync_on) {
        int64_t ts = sec * (int64_t)1000000000 + usec * (int64_t)1000;
        procs->vsync(procs, 0, ts);
    }
}

int init_primary_display(omap_hwc_device_t *hwc_dev)
{
    if (hwc_dev->displays[HWC_DISPLAY_PRIMARY]) {
        ALOGE("Display %d is already connected", HWC_DISPLAY_PRIMARY);
        return -EBUSY;
    }

	const char *modules[] = { "omapdrm" };
	int drm_fd;
	int i, n;
	drmModeResPtr resources;
	drmModeConnector *connector;
	drmModeEncoder *encoder;
	drmModeModeInfoPtr mode;
	uint32_t possible_crtcs;
	IMG_gralloc_module_public_t *m = NULL;

	 /* Open DRM device */
	for (i = 0; i < ARRAY_SIZE(modules); i++) {
		drm_fd = drmOpen(modules[i], NULL);
		if (drm_fd >= 0) {
			ALOGI("Open %s drm device (%d)\n", modules[i], drm_fd);
			break;
		}
	}

	if (drm_fd < 0) {
		ALOGE("Failed to open DRM: %s\n", strerror(errno));
		return -EINVAL;
	}

	resources = drmModeGetResources(drm_fd);
	if (!resources) {
		ALOGE("Failed to get resources: %s\n", strerror(errno));
		goto close;
	}

	connector = drmModeGetConnector(drm_fd, resources->connectors[HWC_DISPLAY_PRIMARY]);
	if (!connector) {
		ALOGE("No connector for DISPLAY %d\n", HWC_DISPLAY_PRIMARY);
		goto free_ressources;
	}

	mode = &connector->modes[0];

	encoder = drmModeGetEncoder(drm_fd, connector->encoders[0]);
	if (!encoder) {
		ALOGE("Failed to get encoder\n");
		goto free_connector;
	}

	i = 0;
	n = encoder->possible_crtcs;
	while (!(n & 1)) {
		n >>= 1;
		i++;
	}

	hwc_dev->drm_fd = drm_fd;
    hwc_dev->drm_resources = resources;
	{
		hw_module_t *pmodule = NULL;
		IMG_gralloc_module_public_t *m = NULL;
		hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&pmodule);
		m = (IMG_gralloc_module_public_t *)(pmodule);
		ALOGI("Set drm fd (%d) to gralloc", drm_fd);
		m->drm_fd = drm_fd;
	}


    //allocate the display object
    init_primary_lcd_display(hwc_dev, mode->hdisplay, mode->vdisplay, NULL);

    display_t *display = hwc_dev->displays[HWC_DISPLAY_PRIMARY];
    display->disp_link.con = connector;
    display->disp_link.enc = encoder;
    display->disp_link.crtc_id = resources->crtcs[i];
    display->disp_link.mode = mode;
    display->disp_link.evctx.version = DRM_EVENT_CONTEXT_VERSION;
    display->disp_link.evctx.vblank_handler = vblank_handler;
    display->disp_link.ctx = hwc_dev;
    display->role = DISP_ROLE_PRIMARY;
    display->opmode = DISP_MODE_PRESENTATION;
    display->mgr_ix = 0;
    display->blanked = true;

    set_primary_display_transform_matrix(hwc_dev);

    primary_display_t *primary = get_primary_display_info(hwc_dev);

    if (!primary) {
        remove_display(hwc_dev, HWC_DISPLAY_PRIMARY);
        return -ENODEV;
    }

    if (use_sw_vsync()) {
        init_sw_vsync(hwc_dev);
        primary->use_sw_vsync = true;
    }

    /* Use default value in case some of requested display parameters missing */
    primary->xpy = 1.0f;

    /* get the board specific clone properties */
    /* eg: 0:0:1280:720 */
    char value[PROPERTY_VALUE_MAX];
    if (property_get("persist.hwc.mirroring.region", value, "") <= 0 ||
        sscanf(value, "%d:%d:%d:%d",
           &primary->mirroring_region.left, &primary->mirroring_region.top,
           &primary->mirroring_region.right, &primary->mirroring_region.bottom) != 4 ||
           primary->mirroring_region.left >= primary->mirroring_region.right ||
           primary->mirroring_region.top >= primary->mirroring_region.bottom) {
        struct hwc_rect fb_region = { .right = mode->hdisplay, .bottom = mode->vdisplay };
        primary->mirroring_region = fb_region;
    }
    ALOGI("clone region is set to (%d,%d) to (%d,%d)",
            primary->mirroring_region.left, primary->mirroring_region.top,
            primary->mirroring_region.right, primary->mirroring_region.bottom);

    return 0;


free_connector:
	drmModeFreeConnector(connector);
free_ressources:
	drmModeFreeResources(resources);
close:
	drmClose(drm_fd);
	return -1;
}

int update_display(omap_hwc_device_t *ctx, int disp,
        hwc_display_contents_1_t *display)
{
    int ret = 0;
    int width = 0, height = 0;
    uint32_t fb = 0;
    kms_display_t *kdisp = &(ctx->displays[disp]->disp_link);
    int i;

    if (!kdisp->con)
        return 0;

    int nLayers = display->numHwLayers;
    hwc_layer_1_t *layers = &display->hwLayers[0];
    hwc_layer_1_t *target = NULL;
    for (i = 0; i < nLayers; i++) {
        if (layers[i].compositionType == HWC_FRAMEBUFFER_TARGET)
            target = &layers[i];
    }
    if (!target) {
        ALOGE("No target");
        return -EINVAL;
    }

    IMG_native_handle_t const *hnd = (IMG_native_handle_t const *)(target->handle);

    width = hnd->iWidth;
    height = hnd->iHeight;

    uint32_t bo[4] = { 0 };
    uint32_t pitch[4] = { width * 4}; //stride
    uint32_t offset[4] = { 0 };


	/*FIXME: Get gem handle from dma buf fd */

    ret = drmPrimeFDToHandle (ctx->drm_fd, hnd->fd[0], &bo[0]);
    if (ret) {
        ALOGE("Failed to get fd for DUMB buffer %s", strerror(errno));
        return ret;
    }


    ret = drmModeAddFB2(ctx->drm_fd, width, height, DRM_FORMAT_ARGB8888,
            bo, pitch, offset, &fb, 0);
    if (ret) {
        ALOGE("cannot create framebuffer (%d): %m\n",errno);
        return ret;
    }

    ret = drmModeSetCrtc(ctx->drm_fd, kdisp->crtc_id, fb, 0, 0,
                &kdisp->con->connector_id, 1, kdisp->mode);
    if (ret) {
        ALOGE("cannot set CRTC for connector %u (%d): %m\n", kdisp->con->connector_id, ret);
        return ret;
    }

    /* Clean up */
    if (kdisp->last_fb) {
        drmModeFBPtr lastFBPtr;
        uint32_t handle;
        lastFBPtr = drmModeGetFB(ctx->drm_fd, kdisp->last_fb);
        handle = lastFBPtr->handle;
        drmModeRmFB(ctx->drm_fd, kdisp->last_fb);
        struct drm_gem_close close_args;
        close_args.handle = handle;
        ret = drmIoctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_args);
        if(ret) {
            ALOGE("Failed to release buffer (Handle = 0x%x): %d\n", handle, ret);
            return ret;
        }
    }
    kdisp->last_fb = fb;


    return 0;
}


void reset_primary_display(omap_hwc_device_t *hwc_dev)
{
    int ret;
    display_t *display = hwc_dev->displays[HWC_DISPLAY_PRIMARY];
    if (!display)
        return;

     /* Blank and unblank fd to make sure display is properly programmed on boot.
     * This is needed because the bootloader can not be trusted.
     */
    blank_display(hwc_dev, HWC_DISPLAY_PRIMARY);
    unblank_display(hwc_dev, HWC_DISPLAY_PRIMARY);
}

primary_display_t *get_primary_display_info(omap_hwc_device_t *hwc_dev)
{
    display_t *display = hwc_dev->displays[HWC_DISPLAY_PRIMARY];
    primary_display_t *primary = NULL;

    if (is_lcd_display(hwc_dev, HWC_DISPLAY_PRIMARY))
        primary = &((primary_lcd_display_t*)display)->primary;

    return primary;
}

int add_external_hdmi_display(omap_hwc_device_t *hwc_dev)
{
    if (hwc_dev->displays[HWC_DISPLAY_EXTERNAL]) {
        ALOGE("Display %d is already connected", HWC_DISPLAY_EXTERNAL);
        return -EBUSY;
    }

	drmModeConnector *connector;
	drmModeEncoder *encoder;
	drmModeModeInfoPtr mode;
	uint32_t possible_crtcs;

    int err, i, n, j;

    drmSetMaster(hwc_dev->drm_fd);

	connector = drmModeGetConnector(hwc_dev->drm_fd, hwc_dev->drm_resources->connectors[HWC_DISPLAY_EXTERNAL]);
	if (!connector) {
		ALOGE("No connector for DISPLAY %d\n", HWC_DISPLAY_EXTERNAL);
		return -1;
	}

    //set the mode for connector to fix 640x480 resolution
	mode = &connector->modes[0];

	encoder = drmModeGetEncoder(hwc_dev->drm_fd, connector->encoders[0]);
	if (!encoder) {
		ALOGE("Failed to get encoder\n");
		goto free_connector;
	}

    drmDropMaster(hwc_dev->drm_fd);

	i = 0;
	n = encoder->possible_crtcs;
	while (!(n & 1)) {
		n >>= 1;
		i++;
	}

    err = allocate_display(sizeof(external_hdmi_display_t), HDMI_DISPLAY_CONFIGS, &hwc_dev->displays[HWC_DISPLAY_EXTERNAL]);
    if (err)
        return err;

    //allocate the display object
    err = init_hdmi_display(hwc_dev, HWC_DISPLAY_EXTERNAL);
    if (err) {
        remove_external_hdmi_display(hwc_dev);
        return err;
    }

    display_t *display = hwc_dev->displays[HWC_DISPLAY_EXTERNAL];
    display->disp_link.con = connector;
    display->disp_link.enc = encoder;
    display->disp_link.crtc_id = hwc_dev->drm_resources->crtcs[i];

    //Change the mode to PREFERRED_HDMI_RESOLUTION

	for (j = 0; j < connector->count_modes; j++) {
		mode = &connector->modes[j];
		if (!strcmp(mode->name, PREFERRED_HDMI_RESOLUTION)) {
            break;
		}
    }

    display->disp_link.mode = mode;
    display->disp_link.evctx.version = DRM_EVENT_CONTEXT_VERSION;
    display->disp_link.evctx.vblank_handler = vblank_handler;
    display->disp_link.ctx = hwc_dev;
    display->role = DISP_ROLE_EXTERNAL;
    display->opmode = DISP_MODE_PRESENTATION;
    display->type = DISP_TYPE_HDMI;
    display->mgr_ix = 1;
    display->blanked = true;

    /* SurfaceFlinger currently doesn't unblank external display on reboot.
     * Unblank HDMI display by default.
     * See SurfaceFlinger::readyToRun() function.
     */
    display->update_transform = true;

    //IMG_framebuffer_device_public_t *fb_dev = hwc_dev->fb_dev[HWC_DISPLAY_EXTERNAL];
    uint32_t xres = HDMI_FB_WIDTH;
    uint32_t yres = HDMI_FB_HEIGHT;

    // TODO: Verify that HDMI supports xres x yres
    // TODO: Set HDMI resolution? What if we need to do docking of 1080p i.s.o. Presentation?

    setup_hdmi_config(&display->configs[0], xres, yres, &display->disp_link);

    external_hdmi_display_t *ext_hdmi = (external_hdmi_display_t*)display;
 
    /* check set props */
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.hwc.avoid_mode_change", value, "1");
    ext_hdmi->avoid_mode_change = atoi(value) > 0;

    return 0;


free_connector:
	drmModeFreeConnector(connector);
	return -1;
}

void remove_external_hdmi_display(omap_hwc_device_t *hwc_dev)
{
    display_t *display = hwc_dev->displays[HWC_DISPLAY_EXTERNAL];
    if (!display) {
        ALOGW("Failed to remove non-existent display %d", HWC_DISPLAY_EXTERNAL);
        return;
    }

    remove_display(hwc_dev, HWC_DISPLAY_EXTERNAL);
}

static int get_layer_stack(omap_hwc_device_t *hwc_dev, int disp, uint32_t *stack)
{
    return 0;
}

static uint32_t get_display_mode(omap_hwc_device_t *hwc_dev, int disp)
{
    if (!is_valid_display(hwc_dev, disp))
        return DISP_MODE_INVALID;

    if (disp == HWC_DISPLAY_PRIMARY)
        return DISP_MODE_PRESENTATION;

    display_t *display = hwc_dev->displays[disp];

    if (display->type == DISP_TYPE_UNKNOWN)
        return DISP_MODE_INVALID;

    if (!display->contents)
        return DISP_MODE_INVALID;

        return DISP_MODE_LEGACY;

    uint32_t primaryStack, stack;
    int err;

    err = get_layer_stack(hwc_dev, HWC_DISPLAY_PRIMARY, &primaryStack);
    if (err)
        return DISP_MODE_INVALID;

    err = get_layer_stack(hwc_dev, disp, &stack);
    if (err)
        return DISP_MODE_INVALID;

    /* if the secondary stack is not yet initialized by the SurfaceFlinger
    * let's assume it is in the default mode, same as the primary stack
    */
    if ((int)stack < 0)
        stack = primaryStack;

    if (stack != primaryStack)
        return DISP_MODE_PRESENTATION;

    return DISP_MODE_LEGACY;
}


void set_display_contents(omap_hwc_device_t *hwc_dev, size_t num_displays, hwc_display_contents_1_t **displays) {
    size_t i;

    if (num_displays > MAX_DISPLAYS)
        num_displays = MAX_DISPLAYS;

    for (i = 0; i < num_displays; i++) {
        if (hwc_dev->displays[i]) {
            display_t *display = hwc_dev->displays[i];
            display->contents = displays[i];

            if (i != HWC_DISPLAY_PRIMARY) {
                uint32_t mode = get_display_mode(hwc_dev, i);

                if (display->opmode != mode) {
                    display->opmode = mode;
                    display->update_transform = true;
                }
            }
        }
    }

    for ( ; i < MAX_DISPLAYS; i++) {
        if (hwc_dev->displays[i])
            hwc_dev->displays[i]->contents = NULL;
    }

    update_primary_display_orientation(hwc_dev);
}

int get_external_display_id(omap_hwc_device_t *hwc_dev)
{
    size_t i;
    int disp = -1;

    for (i = HWC_DISPLAY_EXTERNAL; i < MAX_DISPLAYS; i++) {
        if (hwc_dev->displays[i] && hwc_dev->displays[i]->type != DISP_TYPE_UNKNOWN) {
            disp = i;
            break;
        }
    }

    return disp;
}

int get_display_configs(omap_hwc_device_t *hwc_dev, int disp, uint32_t *configs, size_t *numConfigs)
{
    if (!numConfigs)
        return -EINVAL;

    if (*numConfigs == 0)
        return 0;

    if (!configs || !is_valid_display(hwc_dev, disp))
        return -EINVAL;

    display_t *display = hwc_dev->displays[disp];
    size_t num = display->num_configs;
    uint32_t c;

    if (num > *numConfigs)
        num = *numConfigs;

    for (c = 0; c < num; c++)
        configs[c] = c;

    *numConfigs = num;

    return 0;
}

int get_display_attributes(omap_hwc_device_t *hwc_dev, int disp, uint32_t cfg, const uint32_t *attributes, int32_t *values)
{
    if (!attributes || !values)
        return 0;

    if (!is_valid_display(hwc_dev, disp))
        return -EINVAL;

    display_t *display = hwc_dev->displays[disp];

    if (cfg >= display->num_configs)
        return -EINVAL;

    const uint32_t *attribute = attributes;
    int32_t *value = values;
    display_config_t *config = &display->configs[cfg];

    while (*attribute != HWC_DISPLAY_NO_ATTRIBUTE) {
        switch (*attribute) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            *value = 1000000000 / config->fps;
            break;
        case HWC_DISPLAY_WIDTH:
            *value = config->xres;
            break;
        case HWC_DISPLAY_HEIGHT:
            *value = config->yres;
            break;
        case HWC_DISPLAY_DPI_X:
            *value = 1000 * config->xdpi;
            break;
        case HWC_DISPLAY_DPI_Y:
            *value = 1000 * config->ydpi;
            break;
        }

        attribute++;
        value++;
    }

    return 0;
}

bool is_valid_display(omap_hwc_device_t *hwc_dev, int disp)
{
    if (disp < 0 || disp > MAX_DISPLAY_ID || !hwc_dev->displays[disp])
        return false;

    return true;
}

bool is_supported_display(omap_hwc_device_t *hwc_dev, int disp)
{
    return is_valid_display(hwc_dev, disp) && hwc_dev->displays[disp]->type != DISP_TYPE_UNKNOWN;
}

bool is_active_display(omap_hwc_device_t *hwc_dev, int disp)
{
    return is_valid_display(hwc_dev, disp) && hwc_dev->displays[disp]->contents;
}

bool is_lcd_display(omap_hwc_device_t *hwc_dev, int disp)
{
    return is_valid_display(hwc_dev, disp) && hwc_dev->displays[disp]->type == DISP_TYPE_LCD;
}

bool is_hdmi_display(omap_hwc_device_t *hwc_dev, int disp)
{
    return is_valid_display(hwc_dev, disp) && hwc_dev->displays[disp]->type == DISP_TYPE_HDMI;
}

int blank_display(omap_hwc_device_t *hwc_dev, int disp)
{
    if (!is_valid_display(hwc_dev, disp))
        return -ENODEV;

    display_t *display = hwc_dev->displays[disp];
    int err = 0;

    display->blanked = true;

    switch (display->type) {
    case DISP_TYPE_LCD:
            err = 0;
        break;
    default:
        err = -ENODEV;
        break;
    }

    if (err)
        ALOGW("Failed to blank display %d (%d)", disp, err);

    return err;
}

int unblank_display(omap_hwc_device_t *hwc_dev, int disp)
{
    if (!is_valid_display(hwc_dev, disp))
        return -ENODEV;

    display_t *display = hwc_dev->displays[disp];
    int err = 0;

    display->blanked = false;

    switch (display->type) {
    case DISP_TYPE_LCD:
            err = 0;
        break;
    default:
        err = -ENODEV;
        break;
    }

    if (err)
        ALOGW("Failed to unblank display %d (%d)", disp, err);

    return err;
}

int disable_display(omap_hwc_device_t *hwc_dev, int disp)
{
    if (!is_valid_display(hwc_dev, disp))
        return -ENODEV;
    /* We can remove display from composition by changing its type to unknown.
     *
     * HACK: Changing active display type is safe here because the only operation we are
     * going to do on this display is remove. At the moment removing does not depend on
     * display type.
     */
    hwc_dev->displays[disp]->type = DISP_TYPE_UNKNOWN;

    return 0;
}

int setup_display_tranfsorm(omap_hwc_device_t *hwc_dev, int disp)
{
    if (!is_valid_display(hwc_dev, disp))
        return -ENODEV;

    return 0;
    display_t *display = hwc_dev->displays[disp];
    int err = 0;

    if (display->role == DISP_ROLE_PRIMARY)
        set_primary_display_transform_matrix(hwc_dev);
    else
        err = setup_external_display_transform(hwc_dev, disp);

    if (!err)
        display->update_transform = false;

    return err;
}

int apply_display_transform(omap_hwc_device_t *hwc_dev, int disp)
{
    if (!is_valid_display(hwc_dev, disp))
        return -ENODEV;

    display_t *display = hwc_dev->displays[disp];
    display_transform_t *transform = &display->transform;

    if (!transform->rotation && !transform->hflip && !transform->scaling)
        return 0;

    composition_t *comp;
    comp = &display->composition;

    uint32_t i;

    for (i = 0; i < hwc_dev->num_ovls; i++) {
        if (comp->planes[i].crtc_id == display->disp_link.crtc_id)
            adjust_drm_plane_to_display(hwc_dev, disp, &comp->plane_info[i]);
    }


    return 0;
}

int validate_display_composition(omap_hwc_device_t *hwc_dev, int disp)
{
    if (!is_valid_display(hwc_dev, disp))
        return -ENODEV;

    /*TBD: Check all the DRM params against display caps */

    return 0;

}

void free_displays(omap_hwc_device_t *hwc_dev)
{
    int i;
    for (i = 0; i < MAX_DISPLAYS; i++)
        free_display(hwc_dev->displays[i]);
}
