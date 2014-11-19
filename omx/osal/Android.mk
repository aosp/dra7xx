LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    src/osal_events.c \
    src/osal_memory.c \
    src/osal_mutex.c \
    src/osal_pipes.c \
    src/osal_semaphores.c \
    src/osal_task.c \

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/inc

LOCAL_SHARED_LIBRARIES := \
    libc \
    libutils \
    liblog \

LOCAL_MODULE:= libosal
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
