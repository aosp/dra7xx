ifeq ($(TARGET_BOARD_PLATFORM), $(filter $(TARGET_BOARD_PLATFORM), jacinto6))

LOCAL_PATH:= $(call my-dir)

include $(call first-makefiles-under,$(LOCAL_PATH))

$(clear-android-api-vars)

endif 

