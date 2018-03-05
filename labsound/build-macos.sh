#!/bin/bash

pushd ./third_party/node-native-video-deps
npm install
popd

xcodebuild -project osx/LabSound.xcodeproj -target LabSound -configuration Release clean
xcodebuild -project osx/LabSound.xcodeproj -target LabSound -configuration Release
