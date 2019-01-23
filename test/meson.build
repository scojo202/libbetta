test_cflags = [
  '-I'+meson.source_root()+'/src',
  '-I../src'
]

demo2 = executable('demo2',
  'demo2.c',
  c_args : test_cflags,
  link_args : ['-lm'],
  dependencies: [
    libbetta_dep
  ],
)

xy = executable('xy-plot-demo',
  'xy-plot-demo.c',
  c_args : test_cflags,
  link_args : ['-lm'],
  dependencies: [
    libbetta_dep
  ],
)

density = executable('density-plot-demo',
  'density-plot-demo.c',
  c_args : test_cflags,
  link_args : ['-lm'],
  dependencies: [
    libbetta_dep
  ],
)

#plot_array = executable('plot-array-demo',
#  'plot-array-demo.c',
#  c_args : test_cflags,
#  link_args : ['-lm'],
#  dependencies: [
#    libbetta_dep
#  ],
#)

stringtest = executable('stringtest',
  'stringtest.c',
  c_args : test_cflags,
  link_args : ['-lm'],
  dependencies: [
    libbetta_dep
  ],
)

testdata = executable('testdata',
  'testdata.c',
  c_args : test_cflags,
  link_args : ['-lm'],
  dependencies: [
    libbetta_dep
  ],
)

meson.source_root()+'/src'
