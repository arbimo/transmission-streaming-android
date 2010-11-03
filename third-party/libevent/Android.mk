LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	buffer.c \
	epoll.c \
	evbuffer.c \
	evdns.c \
	event.c \
	event_tagging.c \
	evrpc.c \
	evutil.c \
	http.c \
	log.c \
	poll.c \
	signal.c \
	strlcpy.c

LOCAL_CFLAGS := -DHAVE_CONFIG_H
LOCAL_C_INCLUDES := $(LOCAL_PATH)/compat

LOCAL_MODULE:= libevent

LOCAL_COPY_HEADERS_TO :=
LOCAL_COPY_HEADERS := \
	event.h \
	evhttp.h \
	evdns.h \
	evrpc.h \
	evutil.h \
	event-config.h

include $(BUILD_STATIC_LIBRARY)
