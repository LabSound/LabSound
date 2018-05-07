#!/bin/bash

pushd ..
npm install native-video-deps
popd

xcodebuild -project osx/LabSound.xcodeproj -target LabSound -configuration Release clean
xcodebuild -project osx/LabSound.xcodeproj -target LabSound -configuration Release
