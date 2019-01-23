
data_headers = [
  'b-data-class.h',
  'b-data-simple.h',
  'b-struct.h'
]

data_sources = [
  'b-data.c',
  'b-data-simple.c',
  'b-struct.c'
]

src_public_headers += files(data_headers)
src_public_sources += files(data_sources)

install_headers(data_headers, subdir: join_paths('libbetta-0.2', 'data'))
