# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

{
  'variables': {
    'generic_headers': [
      '<(DEPTH)/starboard/*.h',
    ],
    'wpe_shared_headers': [
      '<(DEPTH)/third_party/starboard/wpe/shared/*.h',
    ],
    'wpe_platform_headers': [
        '<(DEPTH)/third_party/starboard/wpe/<(wpe_platform_dir)/*.h',
    ],
    'target_dir': '<!(echo $COBALT_INSTALL_DIR)',
    'product': '<(PRODUCT_DIR)/cobalt',
    'output_product_dir' : '<(sysroot)/usr/bin',
    'install_dir' : '<(target_dir)/usr/bin',
    'output_generic_include_dir' : '<(sysroot)/usr/include/starboard',
    'output_wpe_shared_include_dir' : '<(sysroot)/usr/include/third_party/starboard/wpe/shared',
    'output_wpe_platform_include_dir' : '<(sysroot)/usr/include/third_party/starboard/wpe/<(wpe_platform_dir)',
    'conditions': [
      ['final_executable_type == "shared_library"', {
        'product': '<(PRODUCT_DIR)/lib/libcobalt.so',
        'output_product_dir' : '<(sysroot)/usr/lib',
        'install_dir' : '<(target_dir)/usr/lib',
      }],
    ],
  },
  'copies': [
    {
      'destination' : '<(output_generic_include_dir)',
      'files': [
        '<!@pymod_do_main(third_party.starboard.wpe.shared.expand_file_list <(generic_headers))'
      ]
    },
    {
      'destination' : '<(output_wpe_shared_include_dir)',
      'files': [
        '<!@pymod_do_main(third_party.starboard.wpe.shared.expand_file_list <(wpe_shared_headers))'
      ]
    },
    {
      'destination' : '<(output_wpe_platform_include_dir)',
      'files': [
        '<!@pymod_do_main(third_party.starboard.wpe.shared.expand_file_list <(wpe_platform_headers))'
      ]
    },
    {
      'destination' : '<(output_product_dir)',
      'files': [ '<(product)' ]
    },
    {
      'destination' : '<(install_dir)',
      'files': [ '<(product)' ]
    },
  ],
}
