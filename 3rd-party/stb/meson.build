project('stb', 'c')

inc = [
    '.'
]

dep = declare_dependency(
    include_directories: include_directories(inc, is_system: true),
)

meson.override_dependency('stb', dep)
