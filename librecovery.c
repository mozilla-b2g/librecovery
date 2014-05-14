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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cutils/android_reboot.h>
#define LOG_TAG "librecovery"
#include <cutils/log.h>

#include "librecovery.h"

#ifndef ALOGE
#define ALOGE LOGE
#endif

#ifndef ALOGW
#define ALOGW LOGW
#endif

#ifndef ALOGD
#define ALOGD LOGD
#endif

#define RECOVERY_DIR "/cache/recovery"

const char kRecoveryCommand[] = RECOVERY_DIR "/command";
const char kLastInstall[] = RECOVERY_DIR "/last_install";
const char kWipeData[] = "--wipe_data";
const char kUpdatePackage[] = "--update_package";
const char kRebootRecovery[] = "recovery";

const int kWipeDataLength = sizeof(kWipeData) - 1;
const int kUpdatePackageLength = sizeof(kUpdatePackage) - 1;
const int kLastInstallMaxLength = PATH_MAX + 3;

static int
safeWrite(FILE *file, const void *data, size_t size)
{
  size_t written;

  do {
    written = fwrite(data, size, 1, file);
  } while (written == 0 && errno == EINTR);

  if (written == 0 && ferror(file)) {
    ALOGE("Error writing data to file: %s", strerror(errno));
    return -1;
  }

  return written;
}

static int
safeRead(FILE *file, void *data, size_t size)
{
  size_t read;

  do {
    read = fread(data, 1, size, file);
  } while (read < size && errno == EINTR);

  if (read < size && ferror(file)) {
    ALOGE("Error reading data from file: %s", strerror(errno));
    return -1;
  }

  return read;
}

// Write a command to the recovery command file in /cache/recovery/command,
// and reboots to the recovery partition.
static int
execRecoveryCommand(char *command, size_t commandLength)
{
  struct stat recoveryDirStat;
  FILE *commandFile;
  size_t bytesWritten;

  // Ensure the recovery directory exists
  if (mkdir(RECOVERY_DIR, 0770) == -1 && errno != EEXIST) {
    ALOGE("Unable to create recovery directory \"%s\": %s", RECOVERY_DIR,
         strerror(errno));
    return -1;
  }

  commandFile = fopen(kRecoveryCommand, "w");
  if (!commandFile) {
    ALOGE("Unable to open recovery command file \"%s\": %s", kRecoveryCommand,
         strerror(errno));
    return -1;
  }

  if (safeWrite(commandFile, command, commandLength) == -1 ||
      safeWrite(commandFile, "\n", 1) == -1) {
    fclose(commandFile);
    return -1;
  }

  fclose(commandFile);

  // Reboot into the recovery partition
  ALOGD("Rebooting into recovery: %s", command);

  return android_reboot(ANDROID_RB_RESTART2, 0, (char *) kRebootRecovery);
}

int
factoryReset()
{
  // In AOSP's recovery image, "--wipe_data" is the synonym for factory reset
  return execRecoveryCommand((char *) kWipeData, kWipeDataLength);
}

// Some devices use a different mount point for external storage in recovery
// than in the main system. This function just normalizes the path to use the
// recovery mount point. Returns the length of the new path.
//
// Note: The buffer pointed to by destPath must have enough space to hold the
// string replacement.
static int
convertExternalStoragePath(char *srcPath, int srcPathLength, char *destPath)
{
  char *extStorage = getenv("EXTERNAL_STORAGE");
  char *relPath;
  int extStorageLength, recoveryExtStorageLength;
  int relPathLength, destPathLength;

  if (!extStorage) {
    strlcpy(destPath, srcPath, srcPathLength + 1);
    return srcPathLength;
  }

  extStorageLength = strlen(extStorage);
  recoveryExtStorageLength = strlen(RECOVERY_EXTERNAL_STORAGE);
  if (strncmp(srcPath, extStorage, extStorageLength) != 0) {
    strlcpy(destPath, srcPath, srcPathLength + 1);
    return srcPathLength;
  }

  relPath = srcPath + extStorageLength;
  relPathLength = srcPathLength - extStorageLength;

  destPathLength = recoveryExtStorageLength + relPathLength;
  strlcpy(destPath, RECOVERY_EXTERNAL_STORAGE, recoveryExtStorageLength + 1);
  strlcat(destPath, relPath, destPathLength + 1);
  return destPathLength;
}

int
installFotaUpdate(char *updatePath, int updatePathLength)
{
  struct stat updateStat;
  char command[kUpdatePackageLength + 1 + PATH_MAX];
  int prefixLength;

  if (!updatePath) {
    ALOGE("Error: null update path");
    return -1;
  }

  if (updatePathLength <= 0) {
    ALOGE("Error: update path length invalid: %d", updatePathLength);
    return -1;
  }

  if (stat(updatePath, &updateStat) == -1) {
    ALOGE("Error: could not stat update path \"%s\": %s",
         updatePath, strerror(errno));
    return -1;
  }

  if (!S_ISREG(updateStat.st_mode)) {
    ALOGE("Error: update path \"%s\" is not a regular file", updatePath);
    return -1;
  }

  prefixLength = kUpdatePackageLength + 1;
  snprintf(command, prefixLength + 1, "%s=", kUpdatePackage);

  updatePathLength = convertExternalStoragePath(updatePath, updatePathLength,
                                                command + prefixLength);
  return execRecoveryCommand(command, prefixLength + updatePathLength);
}

int
getFotaUpdateStatus(FotaUpdateStatus *status)
{
  // The format of the FOTA last_install file is just:
  // UPDATE_PATH\nRESULT\n
  // UPDATE_PATH is the recovery specific path for the applied update
  // RESULT is 0 for failure, or 1 for success
  struct stat lastInstallStat;
  char lastInstallData[kLastInstallMaxLength];
  char *updateResult, *updatePath, *tokenContext;
  int read, lineNumber = 0;
  FILE *lastInstallFile;

  if (!status) {
    ALOGE("Error: null update status");
    return -1;
  }

  // Initial status values
  status->result = FOTA_UPDATE_UNKNOWN;
  status->updatePath[0] = '\0';

  if (stat(kLastInstall, &lastInstallStat) == -1 ||
      !S_ISREG(lastInstallStat.st_mode)) {
    ALOGW("Couldn't find %s", kLastInstall);
    return 0;
  }

  lastInstallFile = fopen(kLastInstall, "r");
  if (!lastInstallFile) {
    ALOGW("Couldn't open %s", kLastInstall);
    return 0;
  }

  if ((read = safeRead(lastInstallFile, lastInstallData,
                       kLastInstallMaxLength - 1)) <= 0) {
    ALOGW("Couldn't read data from %s", kLastInstall);
    fclose(lastInstallFile);
    return 0;
  }
  lastInstallData[read + 1] = '\0';

  updatePath = strtok_r(lastInstallData, "\n", &tokenContext);
  if (!updatePath) {
    ALOGW("Couldn't read update path from %s", kLastInstall);
    fclose(lastInstallFile);
    return 0;
  }
  strlcpy(status->updatePath, updatePath, read - 2);

  updateResult = strtok_r(NULL, "\n", &tokenContext);
  if (!updateResult) {
    ALOGW("Couldn't read update result from %s", kLastInstall);
    fclose(lastInstallFile);
    return 0;
  }

  if (strncmp(updateResult, "1", 1) == 0) {
    status->result = FOTA_UPDATE_SUCCESS;
  } else {
    status->result = FOTA_UPDATE_FAIL;
  }

  fclose(lastInstallFile);
  return 0;
}
