project('wiiu-title-boot-editor', 'cpp',
    default_options : [
        'warning_level=3',
        'werror=true',
    ],
    subproject_dir: '3rd-party'
)

cxx = meson.get_compiler('cpp')
compiler_version = cxx.version()

if cxx.has_argument('-std=c++23')
    std_flag = '-std=c++23'
elif cxx.has_argument('-std=c++2b')
    std_flag = '-std=c++2b'
elif cxx.has_argument('-std=c++20')
    std_flag = '-std=c++20'
else
    error('Please choose a compiler that supports at least c++20')
endif

add_project_arguments(std_flag, language: 'cpp')


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

has_std_expected = cxx.compiles(
    '''
    #include <expected>
    #if !defined(__cpp_lib_expected)
    #error "std::expected not supported"
    #endif
    ''',
    args : std_flag,
)

imgui_dep = dependency('imgui')
libgl_dep = dependency('GL')
sdl2_dep = dependency('SDL2')
stb_dep = dependency('stb')
ImGuiFileDialog_dep = dependency('ImGuiFileDialog')
curl_dep = dependency('libcurl')
ftpclient_dep = dependency('ftpclient-cpp')

cmake = import('cmake')
fmt_opt = cmake.subproject_options()
fmt_opt.add_cmake_defines({'FMT_MASTER_PROJECT': false})
fmt_dep = cmake.subproject('fmt', options: fmt_opt).dependency('fmt')

deps = [
    libgl_dep,
    imgui_dep,
    ImGuiFileDialog_dep,
    sdl2_dep,
    stb_dep,
    ftpclient_dep,
    curl_dep,
    fmt_dep,
]

if not has_std_expected
    expected_opt = cmake.subproject_options()
    expected_opt.add_cmake_defines({'EXPECTED_BUILD_TESTS': false})
    expected_dep = cmake.subproject('expected', options: expected_opt).dependency('expected')
    deps += expected_dep
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

exe = executable('wiiu-title-boot-editor',
    src_files,
    include_directories: inc,
    dependencies: deps,
    install: true
)
