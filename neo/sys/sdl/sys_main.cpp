#include <SDL.h>

#include <cstdlib>

#include "../../idlib/precompiled.h"
#include "sys_sdl.h"

int main(int argc, char** argv) {
    if (!Sys_SDL_Init()) {
        return EXIT_FAILURE;
    }

    common->Init(argc > 0 ? argc - 1 : 0,
                 argc > 0 ? argv + 1 : nullptr,
                 nullptr);

    for (;;) {
        common->Frame();
    }
}
