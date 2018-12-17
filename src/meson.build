src_header_subdir = join_paths('y-plot','src')

src_public_headers = []
src_public_sources = []

subdir('data')
subdir('plot')

install_headers(['y-data.h','y-plot.h'],subdir: 'libyplot-0.2')

yplot_deps = [libgtk3_dep,libm]

libyplot = shared_library('yplot-0.2',src_public_sources, dependencies: yplot_deps, install: true, install_dir: get_option('libdir'))

libyplot_dep = declare_dependency(dependencies: yplot_deps, link_with: libyplot, include_directories: include_directories('.'),)

yplot_dep_sources = []

pkgg = import('pkgconfig')

pkgg.generate(
    libraries: [libyplot],
      subdirs: [ 'libyplot-0.2'.format(meson.project_version()) ],
      version: meson.project_version(),
         name: 'y-plot',
     filebase: 'y-plot-0.2',
  description: 'YPlot',
     requires: [ 'gtk+-3.0'],
  install_dir: join_paths(get_option('libdir'), 'pkgconfig'),
)

# Introspection
if build_gir
  gir_extra_args = [
    '--quiet',
    '--warn-all',
  ]

  yplot_gir = gnome.generate_gir(libyplot,
                                    sources: src_public_headers + src_public_sources,
                                    namespace: 'YPlot',
                                    nsversion: '0.2',
                                    identifier_prefix: 'Y',
                                    symbol_prefix: 'y',
                                    export_packages: 'yplot',
                                    includes: [ 'GObject-2.0', 'Gio-2.0', 'Gtk-3.0'],
                                    install: true,
                                    extra_args: gir_extra_args)
  yplot_dep_sources += yplot_gir
endif