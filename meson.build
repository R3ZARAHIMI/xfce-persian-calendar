project('persian-calendar', 'c',
  version: '0.1.0',
  meson_version: '>= 0.51.0',
  default_options: ['warning_level=2', 'c_std=gnu99']
)

# Core dependencies
dependency_xfce4panel = dependency('libxfce4panel-2.0', required: true)
dependency_xfce4ui = dependency('libxfce4ui-2', required: true)
dependency_gtk3 = dependency('gtk+-3.0', required: true)
dependency_glib = dependency('glib-2.0', required: true)

cc = meson.get_compiler('c')
libm = cc.find_library('m', required: false)

dependencies = [
  dependency_xfce4panel,
  dependency_xfce4ui,
  dependency_gtk3,
  dependency_glib,
  libm
]

# Source files
sources = [
  'panel-plugin/persian-calendar.c',
  'jalali.c',
  'hijri.c',
  'events_data.c'
]

# Xfce4-panel plugin dynamic module (shared library)
# Note: we explicitly set name_prefix to 'lib' since xfce4-panel expects 'libpersian-calendar.so'
shared_module('persian-calendar',
  sources,
  dependencies: dependencies,
  name_prefix: 'lib',
  install: true,
  install_dir: dependency_xfce4panel.get_variable(pkgconfig: 'libdir') / 'xfce4' / 'panel' / 'plugins'
)

# Copy the desktop file and prepare it for installation
configure_file(
  input: 'persian-calendar.desktop.in',
  output: 'persian-calendar.desktop',
  copy: true,
  install: true,
  install_dir: get_option('datadir') / 'xfce4' / 'panel' / 'plugins'
)
