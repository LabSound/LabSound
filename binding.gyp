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
          'libraries': [
            'LabSound.lib',
            'libnyquist.lib',
          ],
          'library_dirs': [
            '<(module_root_dir)/labsound/build/x64/Release',
            '<(module_root_dir)/labsound/win.vs2017/x64/Release',
          ],
        }],
        ['OS=="mac"', {
          'libraries': [
            '-framework Cocoa',
            '-framework Accelerate',
            '-framework CoreAudio',
            '-framework AudioUnit',
            '-framework AudioToolbox',
            '-llabsound',
            '-lnyquist',
          ],
          'library_dirs': [
            '<(module_root_dir)/labsound/osx/build/Release',
            '<(module_root_dir)/labsound/third_party/libnyquist/build/Release',
          ],
        }],
      ],
      'copies': [
        {
          'destination': '<(module_root_dir)/build/Release/',
          'files': [
          ]
        }
      ],
    }
  ]
}

