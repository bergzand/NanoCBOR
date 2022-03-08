project('NanoCBOR',
        'c',
        license: 'CC0-1.0',
        default_options : ['werror=true', 'c_std=gnu99'])

project_sources = []
inc = [include_directories('include')]

subdir('src')

shared_library_bin_deps = [
  decoder_lib,
  encoder_lib,
]

nanocbor_lib = library('nanocbor', project_sources, include_directories: inc) 


if get_option('enable-examples')
  subdir('examples')
endif
if get_option('enable-tests')
  subdir('tests')
endif

