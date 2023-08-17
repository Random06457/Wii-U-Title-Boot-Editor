project('wiiu-title-boot-editor', 'cpp',
    default_options : [
        # 'warning_level=3',
        # 'cpp_args=-std=c++23',
        # 'werror=true',
        # 'b_sanitize=address'
    ],
    subproject_dir: '3rd-party'
)

cxx = meson.get_compiler('cpp')
compiler_version = cxx.version()

add_project_arguments('-std=c++23', language: 'cpp')

if not cxx.compiles(
    '''
    #include <expected>
    int main() {
        return 0;
    }''',
    args : '-std=c++23',
    )
    error('std::expected not supported.')
endif


warnings = [
    '-Wall',
    '-Wextra',
    '-Werror',
    '-Wshadow',
    '-Wconversion',
]

add_project_arguments(cxx.get_supported_arguments(warnings), language: 'cpp')

src_files = [
    'src/fs.cpp',
    'src/image.cpp',
    'src/main_window.cpp',
    'src/main.cpp',
    'src/sound_player.cpp',
    'src/sound.cpp',
    'src/title_meta.cpp',
]

inc = [
    'src/',
]

fmt_dep = dependency('fmt')
imgui_dep = dependency('imgui')
libgl_dep = dependency('GL')
sdl2_dep = dependency('SDL2')
stb_dep = dependency('stb')
# ImGuiFileDialog_dep = dependency('ImGuiFileDialog')

exe = executable('wiiu-title-boot-editor',
    src_files,
    include_directories: inc,
    dependencies: [
        fmt_dep,
        libgl_dep,
        imgui_dep,
        # ImGuiFileDialog_dep,
        sdl2_dep,
        stb_dep,
    ],
    install: true
)
