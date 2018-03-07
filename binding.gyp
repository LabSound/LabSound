{
  'targets': [
    {
      'target_name': 'webaudio',
      'sources': [
        'main.cpp',
        "<!@(node -p \"require('fs').readdirSync('./lib/src').map(f=>'lib/src/'+f).join(' ')\")"
      ],
      'include_dirs': [
        "<!(node -e \"require('nan')\")",
        '<(module_root_dir)/lib/include',
        '<(module_root_dir)/labsound/include',
        '<(module_root_dir)/labsound/third_party/node-native-video-deps/include',
      ],
      'conditions': [
        ['OS=="linux"', {
          'library_dirs': [
            '<(module_root_dir)/labsound/bin',
            '<(module_root_dir)/labsound/third_party/node-native-video-deps/lib/linux',
          ],
          'libraries': [
            '-lLabSound',
            # '-ljack',
            '-lasound',
            # '-lpulse',
            # '-lpulse-simple',
            '-lavformat',
            '-lavcodec',
            '-lavutil',
            '-lswscale',
            '-lswresample',
          ],
          'ldflags': [
            '-Wl,-Bsymbolic',
          ],
        }],
        ['OS=="win"', {
          'library_dirs': [
            '<(module_root_dir)/labsound/build/x64/Release',
            '<(module_root_dir)/labsound/third_party/node-native-video-deps/lib/win',
          ],
          'libraries': [
            'LabSound.lib',
            'avformat.lib',
            'avcodec.lib',
            'avutil.lib',
            'swscale.lib',
            'swresample.lib',
          ],
          'copies': [
            {
              'destination': '<(module_root_dir)/build/Release/',
              'files': [
                '<(module_root_dir)/labsound/third_party/node-native-video-deps/lib/win/avformat-58.dll',
                '<(module_root_dir)/labsound/third_party/node-native-video-deps/lib/win/avcodec-58.dll',
                '<(module_root_dir)/labsound/third_party/node-native-video-deps/lib/win/avutil-56.dll',
                '<(module_root_dir)/labsound/third_party/node-native-video-deps/lib/win/swscale-5.dll',
                '<(module_root_dir)/labsound/third_party/node-native-video-deps/lib/win/swresample-3.dll',
              ]
            }
          ],
        }],
        ['OS=="mac"', {
        'library_dirs': [
            '<(module_root_dir)/labsound/bin',
            '<(module_root_dir)/labsound/third_party/node-native-video-deps/lib/macos',
          ],
          'libraries': [
            '-framework Cocoa',
            '-framework Accelerate',
            '-framework CoreAudio',
            '-framework AudioUnit',
            '-framework AudioToolbox',
            '-llabsound',
          ],
          'library_dirs': [
            '<(module_root_dir)/labsound/osx/build/Release',
          ],
        }],
      ],
    }
  ]
}

