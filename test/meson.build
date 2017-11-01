test_cflags = [
  '-I'+meson.source_root()+'/src',
]

testdata = executable('demo',
  'demo.c',
  c_args : test_cflags,
  link_args : ['-lm'],
  dependencies: [
    libyplot_dep
  ],
)

testsignals = executable('stringtest',
  'stringtest.c',
  c_args : test_cflags,
  link_args : ['-lm'],
  dependencies: [
    libyplot_dep
  ],
)

meson.source_root()+'/src'
