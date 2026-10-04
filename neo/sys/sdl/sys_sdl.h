#ifndef DOOM3BFG_SYS_SDL_H
#define DOOM3BFG_SYS_SDL_H

#include <SDL.h>

bool Sys_SDL_Init();
bool Sys_SDL_CreateWindowAndContext(const char* title, int width, int height,
                                    int fullscreen = 0, int multiSamples = 0);
bool Sys_SDL_SetScreenMode(int x, int y, int width, int height, int fullscreen);
void Sys_SDL_DestroyWindowAndContext();
void Sys_SDL_SwapBuffers();
bool Sys_SDL_MakeContextCurrent(bool current);
bool Sys_SDL_SetGammaRamp(const Uint16 red[256], const Uint16 green[256],
                          const Uint16 blue[256]);
bool Sys_SDL_SetVSync(int interval);

#endif
