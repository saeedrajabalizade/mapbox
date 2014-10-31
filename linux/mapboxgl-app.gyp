{
  'includes': [
    '../gyp/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'copy_styles',
      'type': 'none',
      'hard_dependency': 1,
      'dependencies': [ '../styles.gyp:touch_styles' ], # required for xcode http://openradar.appspot.com/7232149
      'copies': [{
        'files': [ '../styles/styles' ],
        'destination': '<(PRODUCT_DIR)'
      }],
    },
    {
      'target_name': 'linuxapp',
      'product_name': 'mapbox-gl',
      'type': 'executable',
      'sources': [
        './main.cpp',
        '../platform/default/settings_json.cpp',
        '../platform/default/glfw_view.cpp',
        '../platform/default/log_stderr.cpp',
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
          '<@(glfw3_cflags)',
        ],
      },
      'cflags_cc': [
        '<@(glfw3_cflags)',
        '-I<(boost_root)/include',
      ],
      'variables': {
        'ldflags': [
          '<@(png_ldflags)',
          '<@(sqlite3_ldflags)',
          '<@(glfw3_static_libs)',
          '<@(glfw3_ldflags)',
          '<@(curl_ldflags)',
          '<@(zlib_ldflags)',
        ],
      },
      'conditions': [
        ['OS == "mac"', {
          'xcode_settings': {
            'OTHER_LDFLAGS': [ '<@(ldflags)' ],
          }
        }, {
          'libraries': [ '<@(ldflags)' ],
        }]
      ],
      'dependencies': [
        'copy_styles',
        '../mapboxgl.gyp:mbgl-standalone',
        '../mapboxgl.gyp:mbgl-linux',
        '../mapboxgl.gyp:copy_certificate_bundle',
      ],
    },
  ],
}
