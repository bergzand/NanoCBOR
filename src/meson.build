decoder_source = files('decoder.c')
encoder_source = files('encoder.c')

project_sources += decoder_source
project_sources += encoder_source

encoder_lib = static_library('encoder',
                             encoder_source,
                             include_directories : inc)
decoder_lib = static_library('decoder',
                             decoder_source,
                             include_directories : inc)
