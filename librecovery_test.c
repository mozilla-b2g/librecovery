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
#include <string.h>

#include "librecovery.h"

const char kInstallFotaUpdate[] = "installFotaUpdate";
const char kGetFotaUpdateStatus[] = "getFotaUpdateStatus";
const char kFactoryReset[] = "factoryReset";
const char kUpdatePath[] = "updatePath";

const int kInstallFotaUpdateLength = sizeof(kInstallFotaUpdate) - 1;
const int kGetFotaUpdateStatusLength = sizeof(kGetFotaUpdateStatus) - 1;
const int kFactoryResetLength = sizeof(kFactoryReset) - 1;
const int kUpdatePathLength = sizeof(kUpdatePath) - 1;

static void
usage(char *argv0) {
  fprintf(stderr, "Usage: %s [command] (arg1 arg2 .. argN)\n", argv0);
  fprintf(stderr, "Supported commands:\n");
  fprintf(stderr, "    installFotaUpdate    requires 1 arg: system path to the update.zip\n");
  fprintf(stderr, "    getFotaUpdateStatus  requires 1 arg: \"result\" or \"updatePath\"\n");
  fprintf(stderr, "    factoryReset         no args required\n");
}

char *
fotaUpdateResultToString(FotaUpdateResult result)
{
  switch (result) {
    case FOTA_UPDATE_FAIL: return "fail";
    case FOTA_UPDATE_SUCCESS: return "success";
    case FOTA_UPDATE_UNKNOWN:
    default:
      return "unknown";
  }
}

int
main(int argc, char **argv)
{
  int i;
  char *command;

  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  if (strncmp(argv[1], "-h", 2) == 0 ||
      strncmp(argv[1], "--help", 6) == 0) {
    usage(argv[0]);
    return 0;
  }

  command = argv[1];
  if (strncmp(command, kInstallFotaUpdate, kInstallFotaUpdateLength) == 0) {
    if (argc < 3) {
      fprintf(stderr, "Error: update path not supplied for installFotaUpdate\n");
      usage(argv[0]);
      return 1;
    }

    return installFotaUpdate(argv[2], strlen(argv[2]));
  } else if (strncmp(command, kGetFotaUpdateStatus, kGetFotaUpdateStatusLength) == 0) {
    if (argc < 3) {
      fprintf(stderr, "Error: status type not supplied for getFotaUpdateStatus\n");
      usage(argv[0]);
      return 1;
    }

    FotaUpdateStatus status;
    int result = getFotaUpdateStatus(&status);
    if (strncmp(argv[2], kUpdatePath, kUpdatePathLength) == 0) {
      printf("%s\n", status.updatePath);
    } else {
      printf("%s\n", fotaUpdateResultToString(status.result));
    }
    return result;
  } else if (strncmp(command, kFactoryReset, kFactoryResetLength) == 0) {
    return factoryReset();
  }

  fprintf(stderr, "Error: Unknown command: %s\n", argv[1]);
  usage(argv[0]);
  return 1;
}
