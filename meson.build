project('y-plot','c', version: '0.1.2')

gnome = import('gnome')

libgtk3_dep = dependency('gtk+-3.0', version: '>= 3.10')

comp = meson.get_compiler('c')

libm = comp.find_library('m', required: false)

conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', 'y-plot')

if get_option('enable-introspection')
  gir = find_program('g-ir-scanner', required: false)
  if get_option('default_library') == 'shared'
    build_gir = gir.found() and not meson.is_cross_build()
  else
    build_gir = false
  endif
endif

python3 = import('python3')

subdir('src')
subdir('test')

if get_option('enable-gtk-doc')
  subdir('docs/reference')
endif
