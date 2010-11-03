 
# Building Curl


LOCAL_PATH:=$(call my-dir)

CFLAGS := -Wpointer-arith -Wwrite-strings -Wunused -Winline \
 -Wnested-externs -Wmissing-declarations -Wmissing-prototypes -Wno-long-long \
 -Wfloat-equal -Wno-multichar -Wsign-compare -Wno-format-nonliteral \
 -Wendif-labels -Wstrict-prototypes -Wdeclaration-after-statement \
 -Wno-system-headers -DHAVE_CONFIG_H $(MY_CFLAGS)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/lib/Makefile.in

LOCAL_SRC_FILES := $(addprefix lib/,$(CSOURCES))
LOCAL_CFLAGS += $(CFLAGS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ $(LOCAL_PATH)/lib $(LOCAL_PATH)/../../openssl/include/ external/zlib/

LOCAL_COPY_HEADERS_TO := libcurl
# LOCAL_COPY_HEADERS := $(addprefix third-party/curl/include/curl/,$(HHEADERS))

LOCAL_MODULE:= libcurl

include $(BUILD_STATIC_LIBRARY)
