
data_headers = [
  'y-data-class.h',
  'y-data-simple.h',
  'y-struct.h'
]

data_sources = [
  'y-data.c',
  'y-data-simple.c',
  'y-struct.c'
]

src_public_headers += files(data_headers)
src_public_sources += files(data_sources)

install_headers(data_headers, subdir: join_paths('libyplot-0.2', 'data'))
