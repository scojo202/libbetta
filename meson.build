project('y-plot','c', version: '0.0.0')

gnome = import('gnome')

libgtk3_dep = dependency('gtk+-3.0', version: '>= 3.10')
ydata_dep = dependency('y-data-0.0', version: '>=0.0')

comp = meson.get_compiler('c')

libm = comp.find_library('m', required: false)

conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', 'y-plot')

subdir('src')
subdir('test')

#if get_option('enable-gtk-doc')
#  subdir('docs/reference')
#endif


