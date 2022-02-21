automated_sources = [
  'test_decoder.c',
  'test_encoder.c',
  'main.c'
]

automated_test = executable('test_automated',
  [automated_sources],
  include_directories: inc,
  dependencies: [test_deps],
  link_with: nanocbor_lib,
  )

test('automated test', automated_test)
