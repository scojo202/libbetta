project('libbetta','c', version: '0.2.0', default_options: ['buildtype=debugoptimized'])

gnome = import('gnome')

libgtk3_dep = dependency('gtk+-3.0', version: '>= 3.20')

comp = meson.get_compiler('c')

libm = comp.find_library('m', required: false)

conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', 'libbetta')
conf.set_quoted('PACKAGE_ICONDIR', join_paths(get_option('prefix'), get_option('datadir'), 'libbetta/icons'))

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

configure_file(output: 'config.h', configuration: conf)

subdir('data/icons')
subdir('src')
subdir('test')

if get_option('enable-gtk-doc')
  subdir('docs/reference')
endif
