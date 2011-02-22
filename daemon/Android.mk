LOCAL_PATH:= $(call my-dir)

#
# transmission-daemon
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
 daemon.c watch.c

LOCAL_CFLAGS:= \
	-DEMBEDDED \
	$(MY_CFLAGS) \

LOCAL_LDFLAGS += \
	$(MY_LDFLAGS)

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../third-party \
	$(LOCAL_PATH)/../third-party/libevent \
	$(LOCAL_PATH)/../openssl/include \
	$(LOCAL_PATH)/../third-party/curl/include \
	$(LOCAL_PATH)/../third-party/tcp-feeder

LOCAL_SHARED_LIBRARIES := \
	libcutils 


LOCAL_STATIC_LIBRARIES := \
	libtransmission \
    tcp-feeder \
	libscent \
	libcurl \
	libminiupnp \
	libnatpmp \
	libdht \
	libevent \
	libz \
	libssl \
	libcrypto \


LOCAL_LDLIBS += -llog 

LOCAL_MODULE:=transmission-daemon

include $(BUILD_EXECUTABLE)

#
# transmission-remote
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
 remote.c

LOCAL_CFLAGS:= \
	-DEMBEDDED \
	$(MY_CFLAGS) \

LOCAL_LDFLAGS += \
	$(MY_LDFLAGS)

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../third-party \
	$(LOCAL_PATH)/../third-party/libevent \
	$(LOCAL_PATH)/../openssl/include \
	$(LOCAL_PATH)/../third-party/curl/include

LOCAL_SHARED_LIBRARIES := \
	libcutils 

LOCAL_STATIC_LIBRARIES := \
		libtransmission \
		tcp-feeder \
		libscent \
		libcurl \
		libminiupnp \
		libnatpmp \
		libdht \
		libevent \
		libz \
		libssl \
		libcrypto  

LOCAL_LDLIBS += -llog 

LOCAL_MODULE:=transmission-remote

include $(BUILD_EXECUTABLE)
