src_header_subdir = join_paths('y-plot','src')

src_public_headers = []
src_public_sources = []

src_public_headers += [
  'y-element-view.h',
  'y-element-view-cartesian.h',
  'y-axis-markers.h',
  'y-view-interval.h',
  'y-axis-view.h',
  'y-scatter-view.h',
  'y-density-plot.h',
  'y-plot-widget.h',
  'y-rate-label.h'
]

src_public_sources += [
  'y-element-view.c',
  'y-element-view-cartesian.c',
  'y-axis-markers.c',
  'y-view-interval.c',
  'y-axis-view.c',
  'y-scatter-view.c',
  'y-density-plot.c',
  'y-plot-widget.c',
  'y-rate-label.c'
]

install_headers(src_public_headers,subdir: 'libyplot-0.0')

yplot_deps = [libgtk3_dep,ydata_dep,libm]

libyplot = shared_library('yplot-0.0',src_public_sources, dependencies: yplot_deps, install: true, install_dir: get_option('libdir'))

libyplot_dep = declare_dependency(dependencies: yplot_deps, link_with: libyplot, include_directories: include_directories('.'),)

pkgg = import('pkgconfig')

pkgg.generate(
    libraries: [libyplot],
      subdirs: [ 'libyplot-0.0'.format(meson.project_version()) ],
      version: meson.project_version(),
         name: 'y-plot',
     filebase: 'y-plot-0.0',
  description: 'YPlot',
     requires: [ 'gtk+-3.0', 'y-data-0.0'],
  install_dir: join_paths(get_option('libdir'), 'pkgconfig'),
)
