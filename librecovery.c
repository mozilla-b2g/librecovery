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
#include <cutils/log.h>

#include "librecovery.h"

#define LOG_TAG "librecovery"
#define RECOVERY_DIR "/cache/recovery"

const char kRecoveryCommand[] = RECOVERY_DIR "/command";
const char kWipeData[] = "--wipe_data";
const char kUpdatePackage[] = "--update_package";
const char kRebootRecovery[] = "recovery";

const int kWipeDataLength = sizeof(kWipeData) - 1;
const int kUpdatePackageLength = sizeof(kUpdatePackage) - 1;

static int
safeWrite(FILE *file, const void *data, size_t size)
{
  size_t written;

  do {
    written = fwrite(data, size, 1, file);
  } while (written == 0 && errno == EINTR);

  if (written == 0 && ferror(file)) {
    LOGE("Error writing data to file: %s", strerror(errno));
    return -1;
  }

  return written;
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
  if (mkdir(RECOVERY_DIR, 0777) == -1 && errno != EEXIST) {
    LOGE("Unable to create recovery directory \"%s\": %s", RECOVERY_DIR,
         strerror(errno));
    return -1;
  }

  commandFile = fopen(kRecoveryCommand, "w");
  if (!commandFile) {
    LOGE("Unable to open recovery command file \"%s\": %s", kRecoveryCommand,
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
  LOGD("Rebooting into recovery: %s", command);

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
    LOGE("Error: null update path");
    return -1;
  }

  if (updatePathLength <= 0) {
    LOGE("Error: update path length invalid: %d", updatePathLength);
    return -1;
  }

  if (stat(updatePath, &updateStat) == -1) {
    LOGE("Error: could not stat update path \"%s\": %s",
         updatePath, strerror(errno));
    return -1;
  }

  if (!S_ISREG(updateStat.st_mode)) {
    LOGE("Error: update path \"%s\" is not a regular file", updatePath);
    return -1;
  }

  prefixLength = kUpdatePackageLength + 1;
  snprintf(command, prefixLength + 1, "%s=", kUpdatePackage);

  updatePathLength = convertExternalStoragePath(updatePath, updatePathLength,
                                                command + prefixLength);
  return execRecoveryCommand(command, prefixLength + updatePathLength);
}
