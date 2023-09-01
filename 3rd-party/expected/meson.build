project('expected', 'c')

inc = [
    'include'
]

dep = declare_dependency(
    include_directories: include_directories(inc, is_system: true),
)

meson.override_dependency('expected', dep)
