/* Copyright (c) Imagination Technologies Ltd.
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HAL_PUBLIC_H
#define HAL_PUBLIC_H

/* Authors of third party hardware composer (HWC) modules will need to include
 * this header to access functionality in the gralloc and framebuffer HALs.
 */

#include <hardware/gralloc.h>
#include <hardware/hwcomposer_defs.h>

#define ALIGN(x,a)	(((x) + (a) - 1L) & ~((a) - 1L))
#define HW_ALIGN	32

/* This can be tuned down as appropriate for the SOC.
 *
 * IMG formats are usually a single sub-alloc.
 * Some OEM video formats are two sub-allocs (Y, UV planes).
 * Future OEM video formats might be three sub-allocs (Y, U, V planes).
 */
#define MAX_SUB_ALLOCS 3

/* JB MR1 enables dual display support in Android framework. We continue to
 * use the FB HAL architecture to enable such a support, and for this we need
 * FB1 usage identifier in gralloc.
 */
#ifndef GRALLOC_HARDWARE_FB1
#define GRALLOC_HARDWARE_FB1 "fb1"
#endif

/* Number of displays identifier from hwcomposer defs */
#define NUM_FB_DEVICES HWC_NUM_DISPLAY_TYPES

typedef struct
{
	native_handle_t base;

	/* These fields can be sent cross process. They are also valid
	 * to duplicate within the same process.
	 *
	 * A table is stored within psPrivateData on gralloc_module_t (this
	 * is obviously per-process) which maps stamps to a mapped
	 * PVRSRV_CLIENT_MEM_INFO in that process. Each map entry has a lock
	 * count associated with it, satisfying the requirements of the
	 * Android API. This also prevents us from leaking maps/allocations.
	 *
	 * This table has entries inserted either by alloc()
	 * (alloc_device_t) or map() (gralloc_module_t). Entries are removed
	 * by free() (alloc_device_t) and unmap() (gralloc_module_t).
	 *
	 * As a special case for framebuffer_device_t, framebuffer_open()
	 * will add and framebuffer_close() will remove from this table.
	 */

#define IMG_NATIVE_HANDLE_NUMFDS MAX_SUB_ALLOCS
	/* The `fd' field is used to "export" a meminfo to another process.
	 * Therefore, it is allocated by alloc_device_t, and consumed by
	 * gralloc_module_t. The framebuffer_device_t does not need a handle,
	 * and the special value IMG_FRAMEBUFFER_FD is used instead.
	 */
	int fd[MAX_SUB_ALLOCS];

#define IMG_NATIVE_HANDLE_NUMINTS \
	((sizeof(unsigned long long) / sizeof(int)) + 5 + MAX_SUB_ALLOCS + 1)
	/* A KERNEL unique identifier for any exported kernel meminfo. Each
	 * exported kernel meminfo will have a unique stamp, but note that in
	 * userspace, several meminfos across multiple processes could have
	 * the same stamp. As the native_handle can be dup(2)'d, there could be
	 * multiple handles with the same stamp but different file descriptors.
	 */
	unsigned long long ui64Stamp;

	/* This is used for buffer usage validation when locking a buffer,
	 * and also in WSEGL (for the composition bypass feature).
	 */
	int usage;

	/* In order to do efficient cache flushes we need the buffer dimensions
	 * and format. These are available on the ANativeWindowBuffer,
	 * but the platform doesn't pass them down to the graphics HAL.
	 */
	int iWidth;
	int iHeight;
	int iFormat;
	unsigned int uiBpp;

	/* The ion allocation path doesn't allow for the allocation size and
	 * mapping flags to be communicated cross-process automatically.
	 * Cache these here so we can map buffers in client processes.
	 */
	unsigned int uiAllocSize[MAX_SUB_ALLOCS];
	unsigned int uiFlags;
}
__attribute__((aligned(sizeof(int)),packed)) IMG_native_handle_t;

#if 0//defined(SUPPORT_ANDROID_FRAMEBUFFER_HAL)

typedef struct
{
	framebuffer_device_t base;

	/* The HWC was loaded. post() is no longer responsible for presents */
	int bBypassPost;

	/* HWC path for present posts */
	int (*Post2)(framebuffer_device_t *fb, buffer_handle_t *buffers,
				 int num_buffers, void *data, int data_length);
}
IMG_framebuffer_device_public_t;

#endif /* defined(SUPPORT_ANDROID_FRAMEBUFFER_HAL) */

typedef struct
{
	int l, t, w, h;
}
IMG_write_lock_rect_t;

typedef int (*IMG_buffer_format_compute_params_pfn)(
	unsigned int uiPlane, int *piWidth, int *piHeight,
	int *piStride, int *piVStride, unsigned long *pulPlaneOffset);

typedef struct IMG_buffer_format_public_t
{
	/* Buffer formats are returned as a linked list */
	struct IMG_buffer_format_public_t *psNext;

	/* HAL_PIXEL_FORMAT_... enumerant */
	int iHalPixelFormat;

	/* WSEGL_PIXELFORMAT_... enumerant */
	int iWSEGLPixelFormat;

	/* Friendly name for format */
	const char *const szName;

	/* Bits (not bytes) per pixel */
	unsigned int uiBpp;

	/* Supported HW usage bits. If this is GRALLOC_USAGE_HW_MASK, all usages
	 * are supported. Used for HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED.
	 */
	int iSupportedUsage;

	/* YUV output format */
	int bIsYUVFormat;

	/* TRUE if U/Cb follows Y, FALSE if V/Cr follows Y */
	int bUVCbCrOrdering;

	/* Utility function for adjusting YUV per-plane parameters */
	IMG_buffer_format_compute_params_pfn pfnComputeParams;
}
IMG_buffer_format_public_t;

#if 1//defined(SUPPORT_ANDROID_MEMTRACK_HAL)

#include <hardware/memtrack.h>

typedef struct
{
	/* Base memtrack record, copied to caller */
	struct memtrack_record	base;

	/* Record type, for filtering cached records */
	enum memtrack_type		eType;

	/* Process ID, for filtering cached records */
	pid_t					pid;
}
IMG_memtrack_record_public_t;

#endif /* defined(SUPPORT_ANDROID_MEMTRACK_HAL) */

typedef struct IMG_gralloc_module_public_t
{
	gralloc_module_t base;

#if 0//defined(SUPPORT_ANDROID_FRAMEBUFFER_HAL)
	/* If the framebuffer has been opened, this will point to the
	 * framebuffer device data required by the allocator, WSEGL
	 * modules and composerhal.
	 */
	IMG_framebuffer_device_public_t *psFrameBufferDevice[NUM_FB_DEVICES];
#endif /* defined(SUPPORT_ANDROID_FRAMEBUFFER_HAL) */

	/* This function is deprecated and might be NULL. Do not use it. */
	int (*GetPhyAddrs)(gralloc_module_t const* module,
					   buffer_handle_t handle, void **ppvPhyAddr);

	/* Obtain HAL's registered format list */
	const IMG_buffer_format_public_t *(*GetBufferFormats)(void);

#if 1//defined(SUPPORT_ANDROID_MEMTRACK_HAL)
	int (*GetMemTrackRecords)(struct IMG_gralloc_module_public_t const *module,
							  IMG_memtrack_record_public_t **ppsRecords,
							  size_t *puNumRecords);
#endif /* defined(SUPPORT_ANDROID_MEMTRACK_HAL) */

	/* Custom-blit components in lieu of overlay hardware */
	int (*Blit)(struct IMG_gralloc_module_public_t const *module,
				buffer_handle_t src,
				void *dest[MAX_SUB_ALLOCS], int format);

	int (*Blit2)(struct IMG_gralloc_module_public_t const *module,
				 buffer_handle_t src, buffer_handle_t dest,
				 int w, int h, int x, int y);

	int (*Blit3)(struct IMG_gralloc_module_public_t const *module,
				 unsigned long long ui64SrcStamp, int iSrcWidth,
				 int iSrcHeight, int iSrcFormat, int eSrcRotation,
				 buffer_handle_t dest, int eDestRotation);

	int drm_fd;
}
IMG_gralloc_module_public_t;

/**
 * pixel format definitions
 */

enum {
    /*
     * 0x100 - 0x1FF
     *
     * This range is reserved for pixel formats that are specific to the HAL
     * implementation.  Implementations can use any value in this range to
     * communicate video pixel formats between their HAL modules.  These formats
     * must not have an alpha channel.  Additionally, an EGLimage created from a
     * gralloc buffer of one of these formats must be supported for use with the
     * GL_OES_EGL_image_external OpenGL ES extension.
     */

    /*
     * These are vendor specific pixel formats, by (informal) convention IMGTec
     * formats start from the top of the range, TI formats start from the bottom
     */
    HAL_PIXEL_FORMAT_TI_NV12    = 0x100,
    HAL_PIXEL_FORMAT_TI_UNUSED  = 0x101,
    HAL_PIXEL_FORMAT_TI_NV12_1D = 0x102,
    HAL_PIXEL_FORMAT_TI_Y8      = 0x103,
    HAL_PIXEL_FORMAT_TI_Y16     = 0x104,
    HAL_PIXEL_FORMAT_TI_UYVY    = 0x105,
    HAL_PIXEL_FORMAT_BGRX_8888  = 0x1FF,
};

/* we use private_3 gralloc usage flag for identifying FB1 swapchain */
#ifndef GRALLOC_USAGE_HW_FB1
#define GRALLOC_USAGE_HW_FB1 GRALLOC_USAGE_PRIVATE_3
#else
#error GRALLOC_USAGE_HW_FB1 should only be defined by hal_public.h
#endif

#endif /* HAL_PUBLIC_H */
