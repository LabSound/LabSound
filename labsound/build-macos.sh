#!/bin/bash

pushd ./third_party/node-native-video-deps
npm install
popd

xcodebuild -project osx/LabSound.xcodeproj -target LabSound -configuration Release clean
xcodebuild -project osx/LabSound.xcodeproj -target LabSound -configuration Release

xcodebuild -project third_party/libnyquist/libnyquist.xcodeproj -target libnyquist -configuration Release clean
xcodebuild -project third_party/libnyquist/libnyquist.xcodeproj -target libnyquist -configuration Release
