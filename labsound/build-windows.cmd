pushd ..
cmd /C npm install native-video-deps
popd

msbuild .\win.vs2017\LabSound.sln /p:Configuration=Release /p:Platform=x64 /t:Clean,Build
