#!/bin/bash

pushd ..
npm install native-video-deps
popd

cmake .
make clean
make -j2
