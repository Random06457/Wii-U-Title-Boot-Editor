project('ftpclient-cpp', 'cpp',
    default_options : [
        'warning_level=0',
        'werror=false',
        'cpp_std=c++14'
    ],
)

src = [
    'FTP/CurlHandle.cpp',
    'FTP/FTPClient.cpp',
]

inc = [
    '.'
]

add_project_arguments('-DLINUX', language: 'cpp')

libcurl_dep = dependency('libcurl')

ftpclient = static_library('ftpclient',
    src,
    dependencies: [ libcurl_dep ]
)

dep = declare_dependency(
    link_with: ftpclient,
    include_directories: inc,
)

meson.override_dependency('ftpclient-cpp', dep)
