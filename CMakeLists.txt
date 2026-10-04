cmake_minimum_required(VERSION 3.20)

project(Doom3BFG LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This CMake configuration targets the native Windows engine.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Use a 64-bit MinGW-w64 compiler; the engine target must be x64.")
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This configuration requires GCC/MinGW-w64 for both C and C++.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
endif()

# All runtime artifacts (exe, DLLs, import libs) land directly in bin/, with no
# per-configuration subdirectory. The per-config variables must be overridden too,
# otherwise multi-config generators append Debug/ or Release/ automatically.
set(DOOM_BIN_DIR "${CMAKE_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY "${DOOM_BIN_DIR}")
foreach(DOOM_OUTPUT_KIND IN ITEMS RUNTIME LIBRARY ARCHIVE PDB)
    set(CMAKE_${DOOM_OUTPUT_KIND}_OUTPUT_DIRECTORY "${DOOM_BIN_DIR}")
    foreach(DOOM_CONFIG IN ITEMS Debug Release RelWithDebInfo MinSizeRel ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${DOOM_CONFIG}" DOOM_CONFIG_UPPER)
        set(CMAKE_${DOOM_OUTPUT_KIND}_OUTPUT_DIRECTORY_${DOOM_CONFIG_UPPER} "${DOOM_BIN_DIR}")
    endforeach()
endforeach()

file(GLOB_RECURSE DOOM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/neo/*.c"
    "${CMAKE_SOURCE_DIR}/neo/*.cpp"
)
file(GLOB_RECURSE DOOM_CLASSIC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/doomclassic/doom/*.cpp"
)
file(GLOB DOOM_TIMIDITY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/doomclassic/timidity/*.cpp"
)

list(FILTER DOOM_SOURCES EXCLUDE REGEX
    "/(amplitude/amplitude\\.cpp|d3xp/EndLevel\\.cpp|d3xp/gamesys/Callbacks\\.cpp|d3xp/menus/MenuWidget_DevList\\.cpp|framework/Session\\.cpp|framework/zlib/(example|minigzip)\\.c|sys/sdl/.*|game/.*\\.cpp|renderer/BoundsTrack\\.cpp|renderer/jpeg-6/(jcapistd|jctrans|jload|jmemansi|jmemdos|jmemname|jpegtran)\\.cpp|sys/win32/(win_gamma|win_stats)\\.cpp|ui/GameWindow\\.cpp)$"
)
list(FILTER DOOM_CLASSIC_SOURCES EXCLUDE REGEX
    "/(DoomLeaderboards|Networking|PlayerProfile|Precompiled|doomdef|doomstat|i_main|i_net|i_net_ps3|i_net_xbox|i_video_xbox)\\.cpp$"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(FILTER DOOM_SOURCES EXCLUDE REGEX "/sys/win32/win_cpu\\.cpp$")
endif()

if(NOT DOOM_SOURCES)
    message(FATAL_ERROR "No engine source files were found under neo/.")
endif()

add_executable(doom3 WIN32 ${DOOM_SOURCES} ${DOOM_CLASSIC_SOURCES} ${DOOM_TIMIDITY_SOURCES})

target_include_directories(doom3 PRIVATE
    "${CMAKE_SOURCE_DIR}/neo"
    "${CMAKE_SOURCE_DIR}/neo/renderer/jpeg-6"
    "${CMAKE_SOURCE_DIR}/doomclassic"
    "${CMAKE_SOURCE_DIR}/doomclassic/timidity"
)

target_compile_definitions(doom3 PRIVATE
    WIN32
    _WIN32
    WIN64
    _WIN64
    _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:_DEBUG>
)

target_compile_features(doom3 PRIVATE cxx_std_11)
set_target_properties(doom3 PROPERTIES
    C_EXTENSIONS ON
    CXX_EXTENSIONS ON
    OUTPUT_NAME "Doom3BFG"
    RUNTIME_OUTPUT_DIRECTORY "${DOOM_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${DOOM_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${DOOM_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${DOOM_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${DOOM_BIN_DIR}"
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(doom3 PRIVATE
        -m64
        -Wall
        -Wextra
        -Wno-unused-parameter
        $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11>
        # -g, not -g3: macro debug info pushes the image past ~1 GB and ld 2.44
        # then emits a corrupt .rdata (crash in WinMainCRTStartup)
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
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
    xinput
    dbghelp
    x3daudio
    wsock32
    ws2_32
    xaudio2_9
)

add_custom_command(TARGET doom3 POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DENGINE_EXE=$<TARGET_FILE:doom3>"
        "-DENGINE_BIN=${DOOM_BIN_DIR}"
        "-DMINGW_BIN_DIR=${DOOM_MINGW_BIN_DIR}"
        "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}"
        "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake"
    COMMENT "Copying MinGW/vcpkg runtime DLLs into ${DOOM_BIN_DIR}"
    VERBATIM
)

add_custom_target(clean-bin
    COMMAND "${CMAKE_COMMAND}" "-DENGINE_BIN=${DOOM_BIN_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/CleanBin.cmake"
    COMMENT "Removing generated binaries from bin/ (preserving assets)"
    VERBATIM
)
