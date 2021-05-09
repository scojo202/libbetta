
plot_headers = [
  'b-element-view.h',
  'b-element-view-cartesian.h',
  'b-axis-markers.h',
  'b-view-interval.h',
  'b-axis-view.h',
  'b-density-view.h',
  #'b-rate-label.h',
  'b-scatter-series.h',
  'b-scatter-line-view.h',
  'b-plot-widget.h',
  'b-color-map.h',
  'b-color-bar.h',
  'b-legend.h',
  #'b-scalar-label.h'
]

plot_sources = [
  'b-element-view.c',
  'b-element-view-cartesian.c',
  'b-axis-markers.c',
  'b-view-interval.c',
  'b-axis-view.c',
  'b-density-view.c',
  #'b-rate-label.c',
  'b-scatter-series.c',
  'b-scatter-line-view.c',
  'b-plot-widget.c',
  'b-color-map.c',
  'b-color-bar.c',
  'b-legend.c',
  #'b-scalar-label.c'
]

libbetta_enum_headers += files(['b-axis-view.h','b-scatter-series.h'])

src_public_headers += files(plot_headers)
src_public_sources += files(plot_sources)

install_headers(plot_headers, subdir: join_paths('libbetta-0.4', 'plot'))
