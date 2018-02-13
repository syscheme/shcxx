#!/bin/bash

PATHNAME="libuv"
rm -rf $PATHNAME
mkdir $PATHNAME
cp -R include $PATHNAME
cd $PATHNAME
mkdir lib64
cp ../out/Release/libuv.a lib64
cd ..
tar -cf libuv.tar $PATHNAME
rm -rf $PATHNAME
