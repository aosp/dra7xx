LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_BOARD_PLATFORM), $(filter $(TARGET_BOARD_PLATFORM), jacinto6))
# HAL module implementation, not prelinked and stored in
# hw/<HWCOMPOSE_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/../vendor/lib/hw
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libhardware libdrm libdrm_omap

LOCAL_CFLAGS += -DSUPPORT_ANDROID_MEMTRACK_HAL

LOCAL_C_INCLUDES += \
        hardware/libhardware/gralloc \
        external/libdrm \
        external/libdrm/include/drm \
        hardware/libhardware/include

LOCAL_SRC_FILES := \
    color_fmt.c \
    display.c \
    hwc.c \
    layer.c \
    sw_vsync.c \
    utils.c

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_CFLAGS += -DLOG_TAG=\"ti_hwc\"

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../include

# LOG_NDEBUG=0 means verbose logging enabled
# LOCAL_CFLAGS += -DLOG_NDEBUG=0
include $(BUILD_SHARED_LIBRARY)
endif
