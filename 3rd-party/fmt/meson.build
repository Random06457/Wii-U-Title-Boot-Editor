project('fmt', 'cpp',
    default_options : [
        'warning_level=0',
        'werror=false',
    ],
)

src = [
    'src/format.cc',
]

inc = [
    'include'
]

fmt = static_library('ftpclient',
    src,
    include_directories: inc
)

dep = declare_dependency(
    link_with: fmt,
    include_directories: inc,
)

meson.override_dependency('fmt', dep)
