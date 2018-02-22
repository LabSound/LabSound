{
  'targets': [
    {
      'target_name': 'webaudio',
      'sources': [
        'main.cpp',
      ],
      'include_dirs': [
        "<!(node -e \"require('nan')\")",
        '<(module_root_dir)/labsound/include',
      ],
      'library_dirs': [
        '<(module_root_dir)/labsound/build/x64/Release',
        '<(module_root_dir)/labsound/win.vs2017/x64/Release',
      ],
      'conditions': [
        ['OS=="win"', {
          'libraries': [
            'LabSound.lib',
            'libnyquist.lib',
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

