if(NOT DEFINED ENGINE_BIN OR NOT IS_DIRECTORY "${ENGINE_BIN}")
    message(FATAL_ERROR "ENGINE_BIN must name the existing bin directory.")
endif()

file(GLOB generated_binaries
    "${ENGINE_BIN}/*.exe"
    "${ENGINE_BIN}/*.dll"
    "${ENGINE_BIN}/*.a"
    "${ENGINE_BIN}/*.lib"
    "${ENGINE_BIN}/*.exp"
    "${ENGINE_BIN}/*.ilk"
    "${ENGINE_BIN}/*.pdb"
    "${ENGINE_BIN}/*.manifest"
)

if(generated_binaries)
    file(REMOVE ${generated_binaries})
endif()

# Output is flat; remove per-configuration folders left over from older layouts.
foreach(config IN ITEMS Debug Release RelWithDebInfo MinSizeRel x64 x86)
    if(IS_DIRECTORY "${ENGINE_BIN}/${config}")
        file(REMOVE_RECURSE "${ENGINE_BIN}/${config}")
    endif()
endforeach()
