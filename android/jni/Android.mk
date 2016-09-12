#获取当前目录
LOCAL_PATH := $(call my-dir)
#清除一些变量
include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
#要生成的库名
LOCAL_MODULE   := TestClient
LOCAL_CPPFLAGS := -Wall -O3
LOCAL_CPPFLAGS += -fexceptions
#连接的库
LOCAL_LDLIBS := ../dll/libs/$(TARGET_ARCH_ABI)/libzqcommon.so
LOCAL_CFLAGS += -pie -fPIE
LOCAL_LDFLAGS += -pie -fPIE
LOCAL_C_INCLUDES := ../dll/inc
LOCAL_SRC_FILES := TestClient.cpp 
#生成共享库
include $(BUILD_EXECUTABLE)
