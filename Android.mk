#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
ifneq ($(TARGET_SIMULATOR),true)
	
	DEBUG := -g
  MY_CFLAGS := $(DEBUG) -Wall -g
  LOCAL_LDFLAGS += -g

  MY_PATH := $(call my-dir)

  LOCAL_PATH := $(MY_PATH)
  subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
		openssl \
		third-party/curl \
		third-party/libevent \
		third-party/miniupnp \
		third-party/libnatpmp \
		third-party/dht \
		libtransmission \
		daemon \
	))

  include $(subdirs)
endif
