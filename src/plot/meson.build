
plot_headers = [
  'y-element-view.h',
  'y-element-view-cartesian.h',
  'y-axis-markers.h',
  'y-view-interval.h',
  'y-axis-view.h',
  'y-density-plot.h',
  'y-plot-widget.h',
  'y-rate-label.h',
  'y-scatter-series.h',
  'y-scatter-line-view.h',
  'y-scatter-line-plot.h'
]

plot_sources = [
  'y-element-view.c',
  'y-element-view-cartesian.c',
  'y-axis-markers.c',
  'y-view-interval.c',
  'y-axis-view.c',
  'y-density-plot.c',
  'y-plot-widget.c',
  'y-rate-label.c',
  'y-scatter-series.c',
  'y-scatter-line-view.c',
  'y-scatter-line-plot.c'
]

src_public_headers += files(plot_headers)
src_public_sources += files(plot_sources)

install_headers(plot_headers, subdir: join_paths('libyplot-0.2', 'plot'))
