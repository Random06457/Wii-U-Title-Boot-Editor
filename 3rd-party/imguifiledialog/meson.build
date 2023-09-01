project('ImGuiFileDialog', 'cpp',
    default_options : [
        'warning_level=0',
        'werror=false',
    ],
)

src = [
    'ImGuiFileDialog.cpp',
]

inc = [
    '.'
]

add_project_arguments('-DIMGUI_DEFINE_MATH_OPERATORS', language: 'cpp')

imgui = dependency('imgui')

imguifiledialog = static_library('ImGuiFileDialog',
    src,
    dependencies: [ imgui ]
)

dep = declare_dependency(
    link_with: imguifiledialog,
    include_directories: inc,
)

meson.override_dependency('ImGuiFileDialog', dep)
