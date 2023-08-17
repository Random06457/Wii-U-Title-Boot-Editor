project('imgui', 'cpp')

imgui_src = [
    'backends/imgui_impl_sdl2.cpp',
    'backends/imgui_impl_opengl3.cpp',
    'imgui_draw.cpp',
    'imgui_tables.cpp',
    'imgui_widgets.cpp',
    'imgui.cpp',
]

imgui_inc = [
    '.',
    './backends'
]

sdl2_dep = dependency('SDL2')

imgui = static_library('imgui', imgui_src, dependencies: [ sdl2_dep ])

imgui_dep = declare_dependency(
    link_with: imgui,
    include_directories: imgui_inc,
    dependencies: [ sdl2_dep ],
)

meson.override_dependency('imgui', imgui_dep)
