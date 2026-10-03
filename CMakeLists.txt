cmake_minimum_required(VERSION 3.20)

project(Doom3BFG LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This CMake configuration targets the native Windows engine.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Use a 32-bit MinGW-w64 compiler; the engine target must be x86.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DOOM_BIN_DIR "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DOOM_BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${DOOM_BIN_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${DOOM_BIN_DIR}")

foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} "${DOOM_BIN_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config} "${DOOM_BIN_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config} "${DOOM_BIN_DIR}")
endforeach()

file(GLOB_RECURSE DOOM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/neo/*.c"
    "${CMAKE_SOURCE_DIR}/neo/*.cpp"
)

list(FILTER DOOM_SOURCES EXCLUDE REGEX
    "/(amplitude/amplitude\\.cpp|d3xp/EndLevel\\.cpp|d3xp/gamesys/Callbacks\\.cpp|d3xp/menus/MenuWidget_DevList\\.cpp|framework/Session\\.cpp|framework/zlib/(example|minigzip)\\.c|game/.*\\.cpp|renderer/BoundsTrack\\.cpp|renderer/jpeg-6/(jcapistd|jctrans|jload|jmemansi|jmemdos|jmemname|jpegtran)\\.cpp|sys/win32/(win_gamma|win_stats)\\.cpp|ui/GameWindow\\.cpp)$"
)

if(NOT DOOM_SOURCES)
    message(FATAL_ERROR "No engine source files were found under neo/.")
endif()

add_executable(doom3 WIN32 ${DOOM_SOURCES})

target_include_directories(doom3 PRIVATE
    "${CMAKE_SOURCE_DIR}/neo"
    "${CMAKE_SOURCE_DIR}/neo/renderer/jpeg-6"
)

target_compile_definitions(doom3 PRIVATE
    WIN32
    _WIN32
    _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:_DEBUG>
)

target_compile_features(doom3 PRIVATE cxx_std_11)
set_target_properties(doom3 PROPERTIES
    C_EXTENSIONS ON
    CXX_EXTENSIONS ON
    OUTPUT_NAME "doom3"
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(doom3 PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Release>:-O3>
    )
endif()

get_filename_component(DOOM_MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

target_link_libraries(doom3 PRIVATE
    advapi32
    comctl32
    comdlg32
    dinput8
    dsound
    dxguid
    gdi32
    iphlpapi
    ole32
    oleaut32
    opengl32
    shell32
    user32
    uuid
    version
    wbemuuid
    winmm
    wsock32
    ws2_32
    xaudio2_9
)

add_custom_command(TARGET doom3 POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DENGINE_EXE=$<TARGET_FILE:doom3>"
        "-DENGINE_BIN=$<TARGET_FILE_DIR:doom3>"
        "-DMINGW_BIN_DIR=${DOOM_MINGW_BIN_DIR}"
        "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}"
        "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake"
    VERBATIM
)

add_custom_target(clean-bin
    COMMAND "${CMAKE_COMMAND}" "-DENGINE_BIN=${DOOM_BIN_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/CleanBin.cmake"
    COMMENT "Removing generated binaries from bin/ (preserving assets)"
    VERBATIM
)
