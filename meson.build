project('y-plot','c', version: '0.1.3')

gnome = import('gnome')

libgtk3_dep = dependency('gtk+-3.0', version: '>= 3.10')

comp = meson.get_compiler('c')

libm = comp.find_library('m', required: false)

conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', 'y-plot')
conf.set_quoted('PACKAGE_ICONDIR', join_paths(get_option('prefix'), get_option('datadir'), 'y-plot/icons'))

add_global_arguments([
  '-DHAVE_CONFIG_H',
  '-I' + meson.build_root(), # config.h
  '-D_GNU_SOURCE',
], language: 'c')

build_gir = false

if get_option('enable-introspection')
  gir = find_program('g-ir-scanner', required: false)
  if get_option('default_library') == 'shared'
    build_gir = gir.found() and not meson.is_cross_build()
  endif
endif

python3 = import('python3')

yplot_enum_headers = []

configure_file(output: 'config.h', configuration: conf)

subdir('data/icons')
subdir('src')
subdir('test')

if get_option('enable-gtk-doc')
  subdir('docs/reference')
endif
