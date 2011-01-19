LOCAL_PATH:= $(call my-dir)

#
# libminiupnp
#

include $(CLEAR_VARS)

define build-miniupnpcstrings
$(shell $(LOCAL_PATH)/updateminiupnpcstrings.sh $(LOCAL_PATH)/miniupnpcstrings.h.in $(LOCAL_PATH)/miniupnpcstrings.h  > /dev/null )
endef

LOCAL_SRC_FILES:= \
    connecthostport.c \
    igd_desc_parse.c \
    minisoap.c \
    minissdpc.c \
    miniupnpc.c \
    miniwget.c \
    minixml.c \
    upnpcommands.c \
    upnpreplyparse.c

LOCAL_CFLAGS:= \
	-DNDEBUG \

LOCAL_C_INCLUDES:= \
	\

LOCAL_SHARED_LIBRARIES := \
	\

LOCAL_STATIC_LIBRARIES := \
	\


LOCAL_MODULE:=libminiupnp

include $(call build-miniupnpcstrings)

include $(BUILD_STATIC_LIBRARY)
