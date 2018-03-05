pushd .\third_party\node-native-video-deps
npm install
popd

msbuild .\win.vs2017\LabSound.sln /p:Configuration=Release /p:Platform=x64 /t:Clean,Build
