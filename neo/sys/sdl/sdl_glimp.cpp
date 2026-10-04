#include "../../idlib/precompiled.h"
#include "../../renderer/tr_local.h"
#include "sys_sdl.h"

idCVar r_useOpenGL32(
    "r_useOpenGL32", "1", CVAR_INTEGER,
    "OpenGL profile is selected by the SDL platform build", 0, 2
);

bool GLimp_Init(glimpParms_t parms) {
    if (parms.stereo) {
        common->Printf("SDL GLimp does not support quad-buffer stereo\n");
        return false;
    }

    if (!Sys_SDL_CreateWindowAndContext(
            "Doom 3 BFG", parms.width, parms.height,
            parms.fullScreen, parms.multiSamples)) {
        return false;
    }

    glConfig.isFullscreen = parms.fullScreen;
    glConfig.isStereoPixelFormat = false;
    glConfig.stereoPixelFormatAvailable = false;
    glConfig.nativeScreenWidth = parms.width;
    glConfig.nativeScreenHeight = parms.height;
    glConfig.multisamples = parms.multiSamples;
    glConfig.pixelAspect = 1.0f;
    glConfig.physicalScreenWidthInCentimeters = 50.0f;
    glConfig.displayFrequency = 0;
    glConfig.wgl_extensions_string = "";

    glGetIntegerv(GL_RED_BITS, &glConfig.colorBits);
    glGetIntegerv(GL_DEPTH_BITS, &glConfig.depthBits);
    glGetIntegerv(GL_STENCIL_BITS, &glConfig.stencilBits);

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* extensions = glGetString(GL_EXTENSIONS);
    glConfig.renderer_string = renderer != nullptr
        ? reinterpret_cast<const char*>(renderer) : "";
    glConfig.vendor_string = vendor != nullptr
        ? reinterpret_cast<const char*>(vendor) : "";
    glConfig.version_string = version != nullptr
        ? reinterpret_cast<const char*>(version) : "";
    glConfig.extensions_string = extensions != nullptr
        ? reinterpret_cast<const char*>(extensions) : "";

    const int swapInterval = r_swapInterval.GetInteger() == 2 ? 1
        : r_swapInterval.GetInteger() == 1 ? 1 : 0;
    Sys_SDL_SetVSync(swapInterval);

    return true;
}

bool GLimp_SetScreenParms(glimpParms_t parms) {
    if (parms.stereo || parms.multiSamples != glConfig.multisamples) {
        return false;
    }
    if (!Sys_SDL_SetScreenMode(parms.x, parms.y, parms.width,
                               parms.height, parms.fullScreen)) {
        return false;
    }

    glConfig.isFullscreen = parms.fullScreen;
    glConfig.nativeScreenWidth = parms.width;
    glConfig.nativeScreenHeight = parms.height;
    glConfig.pixelAspect = 1.0f;
    return true;
}

void GLimp_Shutdown() {
    Sys_SDL_DestroyWindowAndContext();
}

void GLimp_SetGamma(unsigned short red[256], unsigned short green[256],
                    unsigned short blue[256]) {
    Sys_SDL_SetGammaRamp(red, green, blue);
}

void GLimp_SwapBuffers() {
    if (r_swapInterval.IsModified()) {
        r_swapInterval.ClearModified();
        const int value = r_swapInterval.GetInteger();
        Sys_SDL_SetVSync(value == 2 || value == 1 ? 1 : 0);
    }
    Sys_SDL_SwapBuffers();
}

GLExtension_t GLimp_ExtensionPointer(const char* name) {
    return reinterpret_cast<GLExtension_t>(SDL_GL_GetProcAddress(name));
}

bool GLimp_SpawnRenderThread(void (*)()) {
    // OpenGL contexts are thread-affine; keep rendering on the main thread for now.
    return false;
}

void* GLimp_BackEndSleep() {
    return nullptr;
}

void GLimp_FrontEndSleep() {
}

void GLimp_WakeBackEnd(void*) {
}

void GLimp_ActivateContext() {
    if (!Sys_SDL_MakeContextCurrent(true)) {
        common->Error("GLimp_ActivateContext: SDL_GL_MakeCurrent failed");
    }
}

void GLimp_DeactivateContext() {
    if (!Sys_SDL_MakeContextCurrent(false)) {
        common->Error("GLimp_DeactivateContext: SDL_GL_MakeCurrent failed");
    }
}

void GLimp_EnableLogging(bool) {
}
