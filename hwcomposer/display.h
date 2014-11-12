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

#ifndef __DISPLAY__
#define __DISPLAY__

#include <stdint.h>
#include <stdbool.h>

#include <hardware/hwcomposer.h>

#include "layer.h"

#include "drm_fourcc.h"
#include "xf86drm.h"
#include "xf86drmMode.h"

#define MAX_DISPLAYS 4
#define MAX_DISPLAY_CONFIGS 32

#define MAX_COMPOSITION_BUFFERS 32
#define MAX_COMPOSITION_LAYERS MAX_COMPOSITION_BUFFERS
#define HWC_DISPLAY_SECONDARY HWC_DISPLAY_EXTERNAL+1

#define MAX_DSS_OVERLAYS 4
#define NUM_NONSCALING_OVERLAYS 1


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

typedef struct omap_hwc_device omap_hwc_device_t;

typedef struct dss_platform_info {
    uint8_t max_xdecim_2d;
    uint8_t max_ydecim_2d;
    uint8_t max_xdecim_1d;

    uint8_t max_ydecim_1d;
    uint32_t fclk;
    uint8_t min_width;
    uint16_t max_width;

    uint16_t max_height;
    uint8_t max_downscale;
    uint16_t integer_scale_ratio_limit;
    uint32_t tiler1d_slot_size;
}dss_platform_info_t;


struct display_transform {
    uint8_t rotation;       /* 90-degree clockwise rotations */
    bool hflip;             /* flip l-r (after rotation) */
    bool scaling;
    hwc_rect_t region;
    float matrix[2][3];
};
typedef struct display_transform display_transform_t;

struct display_config {
    int xres;
    int yres;
    int fps;
    int xdpi;
    int ydpi;
};
typedef struct display_config display_config_t;

enum disp_type {
    DISP_TYPE_UNKNOWN,
    DISP_TYPE_LCD,
    DISP_TYPE_HDMI,
    DISP_TYPE_SEC_LCD,
};

enum disp_op_mode {
    DISP_MODE_INVALID,
    DISP_MODE_LEGACY,
    DISP_MODE_PRESENTATION,
};

enum disp_role {
    DISP_ROLE_PRIMARY,
    DISP_ROLE_EXTERNAL,
    DISP_ROLE_SECONDARY,
};

struct composition {
    buffer_handle_t *buffers;
    uint32_t num_buffers;        /* # of buffers used in composition */

    bool use_sgx;
    bool swap_rb;

    uint32_t tiler1d_slot_size;
    uint32_t ovl_ix_base;        /* index of first overlay used in composition */
    uint32_t wanted_ovls;        /* # of overlays required for current composition */
    uint32_t avail_ovls;         /* # of overlays available for current composition */
    uint32_t scaling_ovls;       /* # of overlays available with scaling caps */
    uint32_t used_ovls;          /* # of overlays used in composition */

    struct drm_mode_set_plane plane_info[4];
    drmModePlane planes[4];
};
typedef struct composition composition_t;

typedef struct kms_display {
    drmModeConnectorPtr con;
    drmModeEncoderPtr enc;
    drmModeCrtcPtr crtc;
    int crtc_id;
    drmModeModeInfoPtr mode;
    drmEventContext evctx;
    drmModeFB   fb;
    uint32_t last_fb;
    int vsync_on;
    struct omap_hwc_device *ctx;
} kms_display_t;

struct display {
    uint32_t num_configs;
    display_config_t *configs;
    uint32_t active_config_ix;

    uint32_t type;    /* enum disp_type */
    uint32_t role;    /* enum disp_role */
    uint32_t opmode;    /* enum disp_mode */

    uint32_t mgr_ix;

    bool blanked;

    hwc_display_contents_1_t *contents;
    layer_statistics_t layer_stats;
    composition_t composition;

    display_transform_t transform;
    bool update_transform;

    kms_display_t disp_link;
    dss_platform_info_t limits;
};
typedef struct display display_t;

struct primary_display {
    bool use_sw_vsync;

    uint32_t orientation;
    float xpy;                      /* pixel ratio for UI */
    hwc_rect_t mirroring_region;    /* region to mirror */
};
typedef struct primary_display primary_display_t;

struct primary_lcd_display {
    display_t lcd;
    primary_display_t primary;
};
typedef struct primary_lcd_display primary_lcd_display_t;

int init_primary_display(omap_hwc_device_t *hwc_dev);
void reset_primary_display(omap_hwc_device_t *hwc_dev);
primary_display_t *get_primary_display_info(omap_hwc_device_t *hwc_dev);
int update_display(omap_hwc_device_t *ctx, int disp, hwc_display_contents_1_t *display);

void set_display_contents(omap_hwc_device_t *hwc_dev, size_t num_displays, hwc_display_contents_1_t **displays);

int get_display_configs(omap_hwc_device_t *hwc_dev, int disp, uint32_t *configs, size_t *numConfigs);
int get_display_attributes(omap_hwc_device_t *hwc_dev, int disp, uint32_t config, const uint32_t *attributes, int32_t *values);

bool is_valid_display(omap_hwc_device_t *hwc_dev, int disp);
bool is_supported_display(omap_hwc_device_t *hwc_dev, int disp);
bool is_active_display(omap_hwc_device_t *hwc_dev, int disp);
bool is_lcd_display(omap_hwc_device_t *hwc_dev, int disp);

int blank_display(omap_hwc_device_t *hwc_dev, int disp);
int unblank_display(omap_hwc_device_t *hwc_dev, int disp);
int disable_display(omap_hwc_device_t *hwc_dev, int disp);

int setup_display_tranfsorm(omap_hwc_device_t *hwc_dev, int disp);
int apply_display_transform(omap_hwc_device_t *hwc_dev, int disp);

int validate_display_composition(omap_hwc_device_t *hwc_dev, int disp);

void free_displays(omap_hwc_device_t *hwc_dev);

bool can_dss_render_all_layers(omap_hwc_device_t *hwc_dev, int disp);
bool can_dss_render_layer(omap_hwc_device_t *hwc_dev, int disp, hwc_layer_1_t *layer);
void adjust_drm_plane_to_layer(hwc_layer_1_t const *layer, int zorder, struct drm_mode_set_plane *plane);
bool can_dss_scale(omap_hwc_device_t *hwc_dev, uint32_t src_w, uint32_t src_h,
                   uint32_t dst_w, uint32_t dst_h, bool is_2d,
                   dss_platform_info_t *limits, uint32_t pclk);

#endif
