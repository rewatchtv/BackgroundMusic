#!/bin/bash
set -ex

TARGET="Rewatch Loopback Device"

xcodebuild -scheme "$TARGET" \
  -configuration Release \
  -enableAddressSanitizer NO \
  -archivePath ./archives \
  BUILD_DIR=./build \
  RUN_CLANG_STATIC_ANALYZER=0 \
  build

cd BGMDriver/build/Release
rm -f "$TARGET.driver.zip"
ditto -c -k --keepParent "$TARGET.driver" "$TARGET.driver.notary.zip"
xcrun notarytool submit "$TARGET.driver.notary.zip" --wait \
  --key ~/.appstoreconnect/private_keys/AuthKey_D3Y66SFGVG.p8 \
  --key-id D3Y66SFGVG \
  --issuer 85c9d415-c05c-45c2-8c49-0da415690893
xcrun stapler staple "$TARGET.driver"
rm -f "$TARGET.driver.zip"
ditto -c -k --keepParent "$TARGET.driver" "$TARGET.driver.zip"

