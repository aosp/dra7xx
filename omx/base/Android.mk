# OMX CORE Library #
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    omx_core/src/OMX_Core.c \
    omx_core/src/OMX_Core_Wrapper.c

LOCAL_C_INCLUDES += \
    frameworks/native/include/media/openmax \
    $(LOCAL_PATH)/omx_core/inc \
    $(LOCAL_PATH)/../osal/inc \
    hardware/ti/dce/ \

LOCAL_SHARED_LIBRARIES := \
    libdl \
    libosal \
    libutils \
    liblog \
    libdce

LOCAL_CFLAGS += -DSTATIC_TABLE
LOCAL_MODULE:= libOMX_Core
LOCAL_MODULE_TAGS:= optional
include $(BUILD_SHARED_LIBRARY)

# OMX Base library #
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
    omx_base_comp/src/omx_base.c \
    omx_base_comp/src/omx_base_callbacks.c \
    omx_base_comp/src/omx_base_internal.c \
    omx_base_comp/src/omx_base_process.c \
    omx_base_dio_plugin/src/omx_base_dio.c \
    omx_base_dio_plugin/src/omx_base_dio_table.c \
    omx_base_dio_plugin/src/omx_base_dio_non_tunnel.c

LOCAL_C_INCLUDES += \
    frameworks/native/include/media/openmax \
    $(LOCAL_PATH)/omx_core/inc \
    $(LOCAL_PATH)/../osal/inc \
    system/core/include/cutils \
    $(LOCAL_PATH)/omx_base_dio_plugin/inc/ \
    $(LOCAL_PATH)/omx_base_comp/inc/ \
    hardware/ti/dce/


LOCAL_SHARED_LIBRARIES := \
    libosal \
    libc \
    liblog \
    libcutils \
    libutils \
    libdce

LOCAL_CFLAGS += -DBUILDOS_ANDROID

LOCAL_MODULE:= libOMX
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
