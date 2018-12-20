#获取当前目录
LOCAL_PATH := $(call my-dir)
#清除一些变量
include $(CLEAR_VARS)
#要生成的库名
LOCAL_MODULE    := libzqcommon 
#指定平台
LOCAL_ARM_MODE := arm
#需要引用的库
LOCAL_LDFLAGS  := -fPIC  -shared  -Wl,-soname=libzqcommon.so -Wl,--no-undefined  -ldl
LOCAL_CPPFLAGS += -fexceptions
#-L$(SYSROOT)/usr/lib -lrt -luuid -pthread
#编译参数
LOCAL_CFLAGS := -Wall -O3 -enable-threads
#定义宏
LOCAL_CFLAGS  += -D__linux__ -D__linux -D_LINUX_  -DZQ_COMMON_ANDROID
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc
#LOCAL_SRC_FILES := ../src/Exception.cpp ../src/InetAddr.cpp ../src/Log.cpp ../src/NativeThreadPool.cpp ../src/Pointer.cpp ../src/RTSPClient.cpp ../src/Semaphore.cpp ../src/Socket.cpp ../src/strHelper.cpp ../src/TCPSocket.cpp ../src/urlstr.cpp ../src/FileLog.cpp ../src/SystemUtils.cpp ../src/TimeUtil.cpp ../src/NativeThread.cpp  ../src/RTSPSession.cpp  ../src/linux_com.cpp
#库对应的源文件
MY_CPP_LIST := $(wildcard $(LOCAL_PATH)/../src/*.cpp)
LOCAL_SRC_FILES := $(MY_CPP_LIST:$(LOCAL_PATH)/%=%)
#生成共享库
include $(BUILD_SHARED_LIBRARY)
