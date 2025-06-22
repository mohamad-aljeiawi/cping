
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := cping_memory
LOCAL_CFLAGS := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w
LOCAL_CPPFLAGS  := -std=c++20
LOCAL_CPPFLAGS += -fno-exceptions -fpermissive -frtti
LOCAL_LDLIBS += -llog

LOCAL_LDFLAGS += -lEGL -lGLESv2 -lGLESv3 -landroid -llog -lz
LOCAL_CPP_FEATURES := exceptions


LOCAL_C_INCLUDES :=$(LOCAL_PATH)/include
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/include/types
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/include/utils
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/include/debug
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/include/native
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/include/imgui
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/include/imgui/backends
LOCAL_C_INCLUDES +=$(LOCAL_C_INCLUDES:$(LOCAL_PATH)/%:=%)

FILE_LIST := $(wildcard $(LOCAL_PATH)/src/*.c*)
FILE_LIST += $(wildcard $(LOCAL_PATH)/src/types/*.c*)
FILE_LIST += $(wildcard $(LOCAL_PATH)/src/debug/*.c*)
FILE_LIST += $(wildcard $(LOCAL_PATH)/src/utils/*.c*)
FILE_LIST += $(wildcard $(LOCAL_PATH)/src/native/*.c*)
FILE_LIST += $(wildcard $(LOCAL_PATH)/src/imgui/*.c*)
FILE_LIST += $(wildcard $(LOCAL_PATH)/src/imgui/backends/*.c*)
LOCAL_SRC_FILES += $(FILE_LIST:$(LOCAL_PATH)/%=%)

include $(BUILD_EXECUTABLE)