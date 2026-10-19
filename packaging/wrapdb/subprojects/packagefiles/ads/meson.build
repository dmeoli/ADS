project(
  'ads',
  'cpp',
  version: '0.1.0',
  license: 'LGPL-3.0-only',
  meson_version: '>=1.1.0',
  default_options: ['cpp_std=c++20'],
)

# header-only: the include directory and the threads of the concurrent
# structures
ads_dep = declare_dependency(
  include_directories: include_directories('include'),
  dependencies: dependency('threads'),
)
meson.override_dependency('ads', ads_dep)

install_subdir(
  'include/ADS',
  install_dir: get_option('includedir'),
)
import('pkgconfig').generate(
  name: 'ads',
  description: 'Algorithms and data structures, header-only C++20',
  url: 'https://github.com/dmeoli/ADS',
)

test(
  'use-meson',
  executable(
    'use-meson',
    'packaging/test/main.cpp',
    dependencies: ads_dep,
  ),
)
