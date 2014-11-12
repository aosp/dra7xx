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

#ifndef __HWC_DEV__
#define __HWC_DEV__

#include <stdint.h>
#include <stdbool.h>

#include <pthread.h>

#include <hardware/hwcomposer.h>

#include "hal_public.h"
#include "display.h"

struct omap_hwc_module {
    hwc_module_t base;

};
typedef struct omap_hwc_module omap_hwc_module_t;

struct omap_hwc_device {
    /* static data */
    hwc_composer_device_1_t device;
    pthread_mutex_t ctx_mutex;
    const hwc_procs_t *cb_procs;

    const struct gralloc_module_t *gralloc;

    int drm_fd;
	drmModeResPtr drm_resources;

    int flags_rgb_order;
    int flags_nv12_only;
    float upscaled_nv12_limit;

    int force_sgx;
    int idle;

    display_t *displays[MAX_DISPLAYS];

    bool ext_disp_state;
    int last_ext_ovls;
    int last_int_ovls;
    int tiler1d_slot_size;
    int num_ovls;
};
typedef struct omap_hwc_device omap_hwc_device_t;

#endif
