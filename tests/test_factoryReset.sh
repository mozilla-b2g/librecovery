#!/bin/bash

test_factory_reset() {
  testname "test_factory_reset"
  $ADB shell "mkdir $WORK_DIR 2>/dev/null"
  # add test file in user data partition
  run_command "echo 'should be wiped out' > $WORK_DIR/shouldbewipedout.txt"
  file_exists $WORK_DIR/shouldbewipedout.txt
  echo start factoryReset
  librecovery_test factoryReset
  wait_for_reboot

  # test file should be removed after factory reset
  file_not_exists $WORK_DIR/shouldbewipedout.txt
}

test_factory_reset
