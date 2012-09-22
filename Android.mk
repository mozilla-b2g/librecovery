# Copyright (C) 2012 Mozilla Foundation
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

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

##
# Device specific configuration. These allow us to use generic code in gecko
# for dealing with recovery based OTA updates
##

RECOVERY_EXTERNAL_STORAGE := /sdcard

## End device specific configuration

LOCAL_MODULE := librecovery
LOCAL_SRC_FILES := librecovery.c
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_CFLAGS := -DRECOVERY_EXTERNAL_STORAGE=\"$(RECOVERY_EXTERNAL_STORAGE)\"
LOCAL_MODULE_TAGS := optional eng
include $(BUILD_SHARED_LIBRARY)
