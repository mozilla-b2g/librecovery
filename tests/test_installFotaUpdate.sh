#!/bin/bash

SYSTEM_UPDATE=$TESTDATA_DIR/system_update.zip

system_cleanup() {
  $ADB remount
  $ADB shell rm "/system/bin/itworks.txt" 2>/dev/null
}

test_system_update_from_data() {
  testname "test_system_update_from_data"
  system_cleanup
  $ADB push $SYSTEM_UPDATE $WORK_DIR/system_update.zip
  librecovery_test installFotaUpdate $WORK_DIR/system_update.zip
  wait_for_reboot

  file_exists "/system/bin/itworks.txt"

  RESULT=$(get_fota_update_status result)
  [[ "$RESULT" != "success" ]] && fail "Unexpected result: $RESULT"

  UPDATE_PATH=$(get_fota_update_status updatePath)
  [[ "$UPDATE_PATH" != "$WORK_DIR/system_update.zip" ]] && \
    fail "Unexpected update path: $UPDATE_PATH"
}

test_system_update_from_sdcard() {
  testname "test_system_update_from_sdcard"
  system_cleanup

  echo "Waiting for /mnt/sdcard to be mounted"
  wait_for_sdcard

  $ADB shell mkdir $SDCARD_WORK_DIR
  $ADB push $SYSTEM_UPDATE $SDCARD_WORK_DIR/system_update.zip
  librecovery_test installFotaUpdate $SDCARD_WORK_DIR/system_update.zip
  wait_for_reboot

  file_exists "/system/bin/itworks.txt"
  RESULT=$(get_fota_update_status result)
  if [[ "$RESULT" != "success" ]]; then
    fail "Unexpected result: $RESULT"
  fi

  # skip the update path here, we don't communicate the device specific
  # make vars to the test harness yet
}

test_system_update_from_data
test_system_update_from_sdcard
