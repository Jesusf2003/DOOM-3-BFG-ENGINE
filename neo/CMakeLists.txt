find_package(SDL2 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

set(DOOM3BFG_OPENGL_PROFILE "COMPATIBILITY" CACHE STRING
    "OpenGL profile requested by the SDL platform (COMPATIBILITY or CORE)")
set_property(CACHE DOOM3BFG_OPENGL_PROFILE PROPERTY STRINGS COMPATIBILITY CORE)
if(NOT DOOM3BFG_OPENGL_PROFILE STREQUAL "COMPATIBILITY" AND
   NOT DOOM3BFG_OPENGL_PROFILE STREQUAL "CORE")
    message(FATAL_ERROR
        "DOOM3BFG_OPENGL_PROFILE must be COMPATIBILITY or CORE")
endif()

add_library(doom3bfg_platform INTERFACE)
target_compile_features(doom3bfg_platform INTERFACE cxx_std_17)
target_compile_definitions(doom3bfg_platform INTERFACE ID_GL_HARDLINK)
if(DOOM3BFG_OPENGL_PROFILE STREQUAL "CORE")
    target_compile_definitions(doom3bfg_platform INTERFACE DOOM3BFG_GL_CORE_PROFILE)
endif()
target_link_libraries(doom3bfg_platform INTERFACE
    SDL2::SDL2
    OpenGL::GL
    GLEW::GLEW
    OpenAL::OpenAL
    ZLIB::ZLIB
)

# Platform bootstrap is built independently while the remaining engine sources are ported.
add_library(doom3bfg_sdl_platform STATIC
    sys/sdl/sys_sdl.cpp
)
target_include_directories(doom3bfg_sdl_platform
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(doom3bfg_sdl_platform
    PUBLIC
        doom3bfg_platform
)

add_library(doom3bfg_sdl_glimp STATIC
    sys/sdl/sdl_glimp.cpp
)
target_include_directories(doom3bfg_sdl_glimp
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(doom3bfg_sdl_glimp
    PUBLIC
        doom3bfg_sdl_platform
)

function(doom3bfg_add_engine_library target)
    set(sources)
    foreach(source_directory IN LISTS ARGN)
        file(GLOB_RECURSE directory_sources CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${source_directory}/*.cpp"
        )
        list(APPEND sources ${directory_sources})
    endforeach()
    list(FILTER sources EXCLUDE REGEX "[/\\\\]win32[/\\\\]")
    if(target STREQUAL "framework")
        list(FILTER sources EXCLUDE REGEX "[/\\\\]framework[/\\\\]Session\\.cpp$")
    endif()
    if(target STREQUAL "renderer")
        list(FILTER sources EXCLUDE REGEX
            "[/\\\\]renderer[/\\\\]jpeg-6[/\\\\](jload|jmemdos|jpegtran|jmemname)\\.cpp$")
    endif()
    if(target STREQUAL "game")
        list(FILTER sources EXCLUDE REGEX
            "[/\\\\]d3xp[/\\\\](EndLevel\\.cpp|gamesys[/\\\\]Callbacks\\.cpp)$")
    endif()
    if(target STREQUAL "doom3bfg_support")
        list(FILTER sources EXCLUDE REGEX "[/\\\\]ui[/\\\\]GameWindow\\.cpp$")
    endif()
    add_library(${target} STATIC ${sources})
    target_include_directories(${target}
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(${target}
        PUBLIC
            doom3bfg_platform
    )
endfunction()

doom3bfg_add_engine_library(idlib idlib)
doom3bfg_add_engine_library(framework framework)
doom3bfg_add_engine_library(renderer renderer)
doom3bfg_add_engine_library(game d3xp)
doom3bfg_add_engine_library(doom3bfg_support aas cm sound swf ui)
target_sources(doom3bfg_support PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/sys/sys_local.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sys/sys_session_local.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sys/sys_session_savegames.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sys/sdl/Session_sdl.cpp"
)

target_link_libraries(framework PUBLIC idlib)
target_link_libraries(renderer PUBLIC idlib framework)
target_link_libraries(game PUBLIC idlib framework renderer doom3bfg_support)

add_executable(Doom3BFG
    sys/sdl/sys_main.cpp
)
set_target_properties(Doom3BFG PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
target_link_libraries(Doom3BFG
    PRIVATE
        idlib
        framework
        renderer
        game
        doom3bfg_support
        doom3bfg_sdl_glimp
        doom3bfg_sdl_platform
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
)

if(BUILD_TESTING)
    add_executable(doom3bfg_sdl_platform_tests
        sys/sdl/tests/sys_sdl_tests.cpp
    )
    target_compile_definitions(doom3bfg_sdl_platform_tests
        PRIVATE
            SDL_MAIN_HANDLED
    )
    target_link_libraries(doom3bfg_sdl_platform_tests
        PRIVATE
            doom3bfg_sdl_platform
    )
    add_test(NAME doom3bfg_sdl_platform_tests COMMAND doom3bfg_sdl_platform_tests)
    set_tests_properties(doom3bfg_sdl_platform_tests PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy"
    )
endif()
