if(NOT DEFINED ENGINE_EXE OR NOT EXISTS "${ENGINE_EXE}")
    message(FATAL_ERROR "ENGINE_EXE must name the built Doom executable.")
endif()

if(NOT DEFINED ENGINE_BIN OR NOT IS_DIRECTORY "${ENGINE_BIN}")
    message(FATAL_ERROR "ENGINE_BIN must name the executable output directory.")
endif()

set(runtime_search_directories)
if(DEFINED MINGW_BIN_DIR AND IS_DIRECTORY "${MINGW_BIN_DIR}")
    list(APPEND runtime_search_directories "${MINGW_BIN_DIR}")
endif()
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(vcpkg_bin_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    if(IS_DIRECTORY "${vcpkg_bin_dir}")
        list(APPEND runtime_search_directories "${vcpkg_bin_dir}")
    endif()
endif()

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${ENGINE_EXE}"
    DIRECTORIES ${runtime_search_directories}
    RESOLVED_DEPENDENCIES_VAR resolved_dependencies
    UNRESOLVED_DEPENDENCIES_VAR unresolved_dependencies
)

file(TO_CMAKE_PATH "$ENV{SystemRoot}" system_root)
string(TOLOWER "${system_root}" system_root_lower)
foreach(dependency IN LISTS resolved_dependencies)
    file(TO_CMAKE_PATH "${dependency}" dependency_path)
    string(TOLOWER "${dependency_path}" dependency_lower)
    string(FIND "${dependency_lower}" "${system_root_lower}/" is_system_dependency)
    if(NOT is_system_dependency EQUAL 0)
        get_filename_component(dependency_name "${dependency}" NAME)
        set(destination "${ENGINE_BIN}/${dependency_name}")
        file(REAL_PATH "${dependency}" resolved_path)
        file(REAL_PATH "${destination}" destination_path BASE_DIRECTORY "${ENGINE_BIN}")
        if(NOT resolved_path STREQUAL destination_path)
            file(COPY_FILE "${dependency}" "${destination}" ONLY_IF_DIFFERENT)
        endif()
    endif()
endforeach()

if(unresolved_dependencies)
    message(WARNING "Runtime DLL dependencies were not resolved: ${unresolved_dependencies}")
endif()
