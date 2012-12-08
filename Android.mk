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

LOCAL_PATH := $(call my-dir)

# Some devices share manifests, so this allows us to control whether or not AOSP
# sees the librecovery build modules when the project has been cloned
ifeq ($(ENABLE_LIBRECOVERY),true)

# Device specific configuration. These allow us to use generic code in gecko
# for dealing with recovery based OTA updates
-include $(LOCAL_PATH)/device/$(TARGET_DEVICE).mk

# Device specific variables listed in device/$(TARGET_DEVICE).mk
DEVICE_CONFIG := RECOVERY_EXTERNAL_STORAGE \
                 SYSTEM_FS_TYPE \
                 SYSTEM_PARTITION_TYPE \
                 SYSTEM_LOCATION

LIBRECOVERY_SRC_FILES := librecovery.c
LIBRECOVERY_CFLAGS := $(foreach name,$(DEVICE_CONFIG),-D$(name)=\"$(value $(name))\")
LIBRECOVERY_ENV_VARS := $(foreach name,$(DEVICE_CONFIG),$(name)="$(value $(name))")

# Directories under tests/data that contain updates to be packaged
LIBRECOVERY_TESTDATA := system_update

.PHONY: librecovery_echo_vars
librecovery_echo_vars:
	@echo TARGET_DEVICE = $(TARGET_DEVICE)
	@echo DEVICE_CONFIG = $(DEVICE_CONFIG)
	@echo RECOVERY_EXTERNAL_STORAGE = $(RECOVERY_EXTERNAL_STORAGE)
	@echo LIBRECOVERY_TESTDATA = $(LIBRECOVERY_TESTDATA)
	@echo LIBRECOVERY_CFLAGS = $(LIBRECOVERY_CFLAGS)
	@echo LIBRECOVERY_ENV_VARS = $(LIBRECOVERY_ENV_VARS)

include $(CLEAR_VARS)
LOCAL_MODULE := librecovery
LOCAL_SRC_FILES := $(LIBRECOVERY_SRC_FILES)
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_CFLAGS := $(LIBRECOVERY_CFLAGS)
LOCAL_MODULE_TAGS := optional eng
include $(BUILD_SHARED_LIBRARY)

# librecovery test harness
include $(CLEAR_VARS)
LOCAL_MODULE := librecovery_test
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_SRC_FILES := librecovery_test.c $(LIBRECOVERY_SRC_FILES)
LOCAL_STATIC_LIBRARIES := libcutils libc
LOCAL_CFLAGS := $(LIBRECOVERY_CFLAGS)
LOCAL_MODULE_TAGS := tests
include $(BUILD_EXECUTABLE)

# librecovery test data -- this uses a local make target instead of an AOSP
# module to avoid being built by default (signing an update zip requires java)
TESTS_DIR := $(LOCAL_PATH)/tests
OUT_DIR := $(shell cd $(call local-intermediates-dir); pwd)

.PHONY: librecovery_testdata
librecovery_testdata: librecovery_test
	mkdir -p $(OUT_DIR)
	export TARGET_DEVICE="$(TARGET_DEVICE)" && \
	export LIBRECOVERY_ENV_VARS="$(DEVICE_CONFIG)" && \
	$(foreach env_var,$(LIBRECOVERY_ENV_VARS),export $(env_var) &&) \
	$(foreach dirname,$(LIBRECOVERY_TESTDATA),$(TESTS_DIR)/create_update.sh $(TESTS_DIR)/data/$(dirname) $(OUT_DIR)/$(dirname).zip &&) \
	echo

endif # $(ENABLE_LIBRECOVERY) == true
