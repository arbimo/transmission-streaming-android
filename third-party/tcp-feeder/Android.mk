LOCAL_PATH:= $(call my-dir)

#
# tcp-feeder
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
 cmd_server.c \
 eventqueue.c \
 tcp-server.c

LOCAL_CFLAGS:= \
	-DEMBEDDED \
	$(MY_CFLAGS) \

LOCAL_LDFLAGS += \
	$(MY_LDFLAGS)

LOCAL_C_INCLUDES:= \

LOCAL_SHARED_LIBRARIES := \


LOCAL_STATIC_LIBRARIES := \


LOCAL_LDLIBS += 

LOCAL_MODULE := tcp-feeder

include $(BUILD_STATIC_LIBRARY)