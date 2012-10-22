#!/bin/bash
#
# Test the various recovery reboot commands to ensure they work properly.
# We use a host shell script harness here since these APIs will cause device
# reboots.
#
# Modified from AOSP bootable/recovery/verify_test.sh.
#
# DO NOT RUN THIS ON A DEVICE YOU CARE ABOUT.  It will mess up your
# system partition.

THIS_DIR=$(cd "`dirname "$0"`"; pwd)
TESTS_DIR=$THIS_DIR/tests
WORK_DIR=/data/local/tmp
TOP_DIR=$(cd "$THIS_DIR/.."; pwd)
TEST_TARGET=$1
SKIP_SDCARD=

# ------------------------

ADB=${ADB:-"adb -d "}

$ADB wait-for-device

SDCARD_WORK_DIR=$($ADB shell echo -n \$EXTERNAL_STORAGE/tmp)

# This currently requires DEVICE from B2G's .config
. $TOP_DIR/.config

PRODUCT_OUT=$TOP_DIR/out/target/product/$DEVICE
TESTDATA_DIR=$PRODUCT_OUT/obj/EXECUTABLES/librecovery_test_intermediates

# run a command on the device; exit with the exit status of the device
# command.
run_command() {
  $ADB shell "$@" \; echo \$? | awk '{if (b) {print a}; a=$0; b=1} END {exit a}'
}

testname() {
  echo
  echo "::: testing $1 :::"
  testname="$1"
}

fail() {
  echo
  echo FAIL: $testname

  if [[ -n "$1" ]]; then
    echo $1
  fi

  echo
  exit 1
}

file_exists() {
  run_command "ls $1" 1>/dev/null 2>/dev/null || ( echo "$1 doesn't exist"; fail )
}

file_not_exists() {
  run_command "ls $1" 1>/dev/null 2>/dev/null && ( echo "$1 shouldn't exist"; fail )
}

cleanup() {
  run_command rm -r $WORK_DIR
  if [[ -z "$SKIP_SDCARD" ]]; then
    wait_for_sdcard
    run_command rm -r $SDCARD_WORK_DIR
  fi
}

$ADB push $PRODUCT_OUT/system/bin/librecovery_test $WORK_DIR/librecovery_test

REBOOT_TIMEOUT=180 # 3 minutes
wait_for_reboot() {
  $ADB wait-for-device &
  WAIT_PID=$!

  (
    sleep $REBOOT_TIMEOUT
    echo "Timed out waiting for reboot (pid: $WAIT_PID)"
    kill $WAIT_PID
  ) &
  SLEEP_PID=$!

  trap "kill $WAIT_PID $SLEEP_PID; exit 5" INT TERM EXIT

  wait $WAIT_PID
  WAIT_RESULT=$?

  trap - INT TERM EXIT

  if [[ "$WAIT_RESULT" -le 128 ]]; then
    kill $SLEEP_PID
    wait $SLEEP_PID 2>/dev/null
  fi

  if [[ "$WAIT_RESULT" != "0" ]]; then
    fail
  fi
}

wait_for_sdcard() {
  while [[ "1" ]]; do
    ($ADB shell vdc volume list | grep '110 sdcard .* 4') > /dev/null
    if [[ "$?" = "0" ]]; then
      break
    else
      sleep 1
    fi
  done
}

librecovery_test() {
  # we expect librecovery_test to cause a reboot, so we don't check the result
  run_command $WORK_DIR/librecovery_test $@
}

get_fota_update_status() {
  $ADB shell $WORK_DIR/librecovery_test getFotaUpdateStatus $@ | tr -d "\r\n"
}

if [[ "$TEST_TARGET" = "factoryReset" ]]; then
  # factory reset don't require to have SD card installed on handset,
  # skip SD card clean-up to prevent infinite loop.
  SKIP_SDCARD=1
  . $TESTS_DIR/test_factoryReset.sh
else
  . $TESTS_DIR/test_installFotaUpdate.sh
fi

cleanup

echo
echo PASS
echo
