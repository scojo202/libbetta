version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())

configure_file(input: 'version.xml.in', output: 'version.xml', configuration: version_conf)

docpath=join_paths(meson.build_root(),'html')

gnome.gtkdoc('betta',
             main_xml: 'betta-docs.xml',
             src_dir:  join_paths(meson.source_root(), 'src'),
             dependencies: libbetta_dep,
             gobject_typesfile: 'betta.types',
             scan_args: [
        	'--rebuild-types',
             ],
             fixxref_args: [
               '--html-dir=@0@'.format(docpath),
             ],
             install: true)
