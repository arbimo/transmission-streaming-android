LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:=  trust/trust_agent.c trust/trust_common.c trust/peer_trustmsg.c trust/trust_update.c ndp/ndp_agent.c utils/netutils_common.c scent_msg.c  

LOCAL_MODULE:= libscent

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS+= -Wall -g -O3  

LOCAL_LDLIBS  += -lpthread -lhardware_legacy

LIBS  += -lhardware_legacy

LOCAL_C_INCLUDES += 	$(LOCAL_PATH) $(LOCAL_PATH)/trust $(LOCAL_PATH)/ndp $(LOCAL_PATH)/utils

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:=  trust/trustAgent_test.c

LOCAL_STATIC_LIBRARIES += libscent

LOCAL_MODULE:= trustagenttest

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS+= -Wall -g -O3  

LOCAL_LDLIBS  += -lpthread

LIBS += libhardware_legacy

LOCAL_C_INCLUDES += 	$(LOCAL_PATH) $(LOCAL_PATH)/trust $(LOCAL_PATH)/ndp $(LOCAL_PATH)/utils

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:=  trust/tu_test_client.c

LOCAL_STATIC_LIBRARIES += libscent

LOCAL_MODULE:= tutestclient

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS+= -Wall -g -O3  

LOCAL_LDLIBS  += -lpthread

LOCAL_C_INCLUDES += 	$(LOCAL_PATH) $(LOCAL_PATH)/trust $(LOCAL_PATH)/ndp $(LOCAL_PATH)/utils

include $(BUILD_EXECUTABLE)
