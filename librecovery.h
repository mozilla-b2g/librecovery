/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/*
 * Copyright (C) 2012 Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _LIBRECOVERY_H
#define _LIBRECOVERY_H

#include <unistd.h>

/**
 * This library provides a high level API for using the recovery partition.
 */

#if __cplusplus
extern "C" {
#endif

/**
 * Perform a factory reset. This function will reboot the device into recovery.
 */
int factoryReset();

/**
 * Install a FOTA update.zip. This function will reboot the device into recovery.
 */
int installFotaUpdate(char *updatePath, int updatePathLength);

typedef enum {
  FOTA_UPDATE_UNKNOWN = 0,
  FOTA_UPDATE_FAIL = 1,
  FOTA_UPDATE_SUCCESS = 2
} FotaUpdateResult;

typedef struct {
  FotaUpdateResult result;
  // The path to the update that was installed
  char updatePath[PATH_MAX];
} FotaUpdateStatus;

/**
 * Get the status of a FOTA update after it has been attempted.
 */
int getFotaUpdateStatus(FotaUpdateStatus *status);

#if __cplusplus
}; // extern "C"
#endif

#endif // _LIBRECOVERY_H
