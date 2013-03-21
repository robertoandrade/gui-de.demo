#!/bin/sh

ARCH="armeabi armeabi-v7a mips x86"
LIBDIR=common/lib.android/libs
INCDIR=common/include


tmpdir=`mktemp -d /tmp/$0.XXXXX`

for arch in $ARCH; do
    ndk-build LOCAL_C_INCLUDES=$INCDIR LOCAL_LDFLAGS=-L$LIBDIR/$arch APP_ABI=$arch
    cp libs/$arch/libsample.so $tmpdir/libsample.so.$arch
done

for arch in $ARCH; do
    cp $tmpdir/libsample.so.$arch libs/$arch/libsample.so
done

rm -rf $tmpdir