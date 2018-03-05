#!/bin/bash

pushd ./third_party/node-native-video-deps
npm install
popd

cmake .
make clean
make -j2
