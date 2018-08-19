#!/bin/bash

# pushd ..
# npm install native-video-deps
# popd

cmake -D CMAKE_TOOLCHAIN_FILE=.\\toolchain -D LUMIN=1 .
# make clean
make -j4