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

/**
 * This library provides a high level API for using the recovery partition.
 * All functions in this library will reboot the device into recovery.
 */

#if __cplusplus
extern "C" {
#endif

/**
 * Perform a factory reset
 */
int factoryReset();

/**
 * Install an OTA update.zip
 */
int otaInstall(char *updatePath, int updatePathLength);

#if __cplusplus
}; // extern "C"
#endif

#endif // _LIBRECOVERY_H
