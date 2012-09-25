#!/bin/bash
#
# Creates an update.zip from a test data directory, using
# environment variables provided by the build system

THIS_DIR=$(cd `dirname "$0"`; pwd)
TOOLS_DIR=$(cd "$THIS_DIR/../tools"; pwd)
TOP_DIR=$(cd "$THIS_DIR/../.."; pwd)

if [[ ! -d "$1" ]]; then
  echo "Not a directory: $1"
  exit 1
fi

UPDATE_DIR=$1
DEST=$2

if [[ ! -n "$DEST" ]]; then
  echo "Missing destination"
  exit 1
fi

UPDATER_SCRIPT_DIR=META-INF/com/google/android
UPDATER_SCRIPT_PATH=$UPDATER_SCRIPT_DIR/updater-script
UPDATE_BINARY_PATH=$UPDATER_SCRIPT_DIR/update-binary
UPDATER_SCRIPT=$UPDATE_DIR/$UPDATER_SCRIPT_PATH

if [[ ! -f "$UPDATER_SCRIPT" ]]; then
  echo "No updater-script at $UPDATER_SCRIPT"
  exit 1
fi

if [[ ! -n "$LIBRECOVERY_ENV_VARS" ]]; then
  echo "LIBRECOVERY_ENV_VARS must list all env vars to replace in the updater-script"
  exit 1
fi

if [[ ! -n "$TARGET_DEVICE" ]]; then
  echo "TARGET_DEVICE must specify the target device"
  exit 1
fi

PATTERNS=
for ENV_VAR in $LIBRECOVERY_ENV_VARS; do
  VALUE=$(eval echo \$$ENV_VAR | sed -e 's/\//\\\//g')
  PATTERNS+="s/%$ENV_VAR%/$VALUE/g; "
done

echo $PATTERNS

TMP_DIR=/tmp/librecovery_update
TMP_UPDATE_DIR=$TMP_DIR/update
rm -rf $TMP_DIR
mkdir -p $TMP_UPDATE_DIR

mkdir -p $TMP_UPDATE_DIR/$UPDATER_SCRIPT_DIR
sed -e "$PATTERNS" $UPDATER_SCRIPT > $TMP_UPDATE_DIR/$UPDATER_SCRIPT_PATH
cp $TOP_DIR/out/target/product/$TARGET_DEVICE/symbols/system/bin/updater \
   $TMP_UPDATE_DIR/$UPDATE_BINARY_PATH

shopt -s extglob
cp -r $UPDATE_DIR/!(META-INF) $TMP_UPDATE_DIR

cd $TMP_UPDATE_DIR
zip -r $TMP_DIR/update.zip * 1>/dev/null || echo "Error creating zip"

# Try to find a "real" JAVA_HOME since we fake it in B2G
if [[ "$JAVA_HOME" =  *fake-jdk-tools ]]; then
  case `uname -s` in
    Darwin)
      export JAVA_HOME=/System/Library/Frameworks/JavaVM.framework/Versions/1.6/Home
      ;;
    *)
      export JAVA_HOME=/usr/lib/jvm/java-6-sun
      ;;
  esac
  export PATH=$JAVA_HOME/bin:$PATH
fi

KEYDIR=$TOP_DIR/build/target/product/security

echo java -Xmx2048m -jar $TOOLS_DIR/signapk.jar -w \
  $KEYDIR/testkey.x509.pem $KEYDIR/testkey.pk8 $TMP_DIR/update.zip $DEST

java -Xmx2048m -jar $TOOLS_DIR/signapk.jar -w \
  $KEYDIR/testkey.x509.pem $KEYDIR/testkey.pk8 $TMP_DIR/update.zip $DEST

echo Update created at $DEST
