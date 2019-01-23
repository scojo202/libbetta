src_header_subdir = join_paths('libbetta','src')

libbetta_header_subdir = 'libbetta-0.2'
libbetta_header_dir = join_paths(get_option('includedir'),libbetta_header_subdir)

src_public_headers = []
src_public_sources = []

libbetta_enum_headers = []

subdir('data')
subdir('plot')

libbetta_enums = gnome.mkenums('libbetta-enums',
      h_template: 'b-plot-enums.h.in',
      c_template: 'b-plot-enums.c.in',
         sources: libbetta_enum_headers,
  install_header: true,
     install_dir: libbetta_header_dir,
)

src_public_sources += [libbetta_enums[0]]
libbetta_generated_headers = [libbetta_enums[1]]

libbetta_sources = [src_public_sources, libbetta_generated_headers]

install_headers(['b-data.h','b-plot.h'],subdir: libbetta_header_subdir)

libbetta_deps = [libgtk3_dep,libm]

libbetta = shared_library('betta-0.2',libbetta_sources, dependencies: libbetta_deps, install: true, install_dir: get_option('libdir'))

libbetta_dep = declare_dependency(dependencies: libbetta_deps, link_with: libbetta, include_directories: include_directories('.'),)

libbetta_dep_sources = []

pkgg = import('pkgconfig')

pkgg.generate(
    libraries: [libbetta],
      subdirs: [ 'libbetta-0.2'.format(meson.project_version()) ],
      version: meson.project_version(),
         name: 'libbetta',
     filebase: 'libbetta-0.2',
  description: 'Betta',
     requires: [ 'gtk+-3.0'],
  install_dir: join_paths(get_option('libdir'), 'pkgconfig'),
)

# Introspection
if build_gir
  gir_extra_args = [
    '--quiet',
    '--warn-all',
  ]

  betta_gir = gnome.generate_gir(libbetta,
                                    sources: src_public_headers + src_public_sources,
                                    namespace: 'Betta',
                                    nsversion: '0.2',
                                    identifier_prefix: 'B',
                                    symbol_prefix: 'b',
                                    includes: [ 'GObject-2.0', 'Gio-2.0', 'Gtk-3.0'],
                                    install: true,
                                    extra_args: gir_extra_args)
  libbetta_dep_sources += betta_gir
endif
