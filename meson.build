project('wiiu-title-boot-editor', 'cpp',
    default_options : [
        'warning_level=3',
        'cpp_args=-std=c++23',
        'werror=true',
    ],
    subproject_dir: '3rd-party'
)

cxx = meson.get_compiler('cpp')
compiler_version = cxx.version()

add_project_arguments('-std=c++23', language: 'cpp')

if not cxx.compiles(
    '''
    #include <expected>
    #if !defined(__cpp_lib_expected)
    #error "std::expected not supported"
    #endif
    ''',
    args : '-std=c++23',
    )
    error('std::expected not supported. Please use another compiler that supports C++23 with std::expected.')
endif


warnings = [
    '-Wno-pedantic',
    '-Wshadow',
    '-Wconversion',
]

add_project_arguments(cxx.get_supported_arguments(warnings), language: 'cpp')

# for some reason -Db_profile doesn't seem to work
if (get_option('profile'))
    add_project_arguments('-pg', language: 'cpp')
    add_project_link_arguments('-pg', language: 'cpp')
endif

src_files = [
    'src/file_dialog.cpp',
    'src/fs.cpp',
    'src/image.cpp',
    'src/main_window.cpp',
    'src/main.cpp',
    'src/sound_player.cpp',
    'src/sound.cpp',
    'src/title_meta.cpp',
    'src/title_mgr.cpp',
]

inc = [
    'src/',
]

imgui_dep = dependency('imgui')
libgl_dep = dependency('GL')
sdl2_dep = dependency('SDL2')
stb_dep = dependency('stb')
ImGuiFileDialog_dep = dependency('ImGuiFileDialog')
curl_dep = dependency('libcurl')
ftpclient_dep = dependency('ftpclient-cpp')


cmake = import('cmake')
opt_var = cmake.subproject_options()
opt_var.add_cmake_defines({'FMT_MASTER_PROJECT': false})
fmt_dep = cmake.subproject('fmt', options: opt_var).dependency('fmt')


exe = executable('wiiu-title-boot-editor',
    src_files,
    include_directories: inc,
    dependencies: [
        fmt_dep,
        libgl_dep,
        imgui_dep,
        ImGuiFileDialog_dep,
        sdl2_dep,
        stb_dep,
        ftpclient_dep,
        curl_dep,
    ],
    install: true
)
