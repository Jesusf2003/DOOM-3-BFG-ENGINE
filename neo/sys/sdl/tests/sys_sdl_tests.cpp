#include <cstdlib>
#include <cstdio>

#include <SDL.h>

#include "../../../idlib/sys/sys_defines.h"
#include "../../../idlib/sys/sys_builddefines.h"
#include "../../../idlib/sys/sys_includes.h"
#include "../../../idlib/sys/sys_assert.h"
#include "../../../idlib/sys/sys_types.h"
#define ID_TIME_T int64
#include "../../sys_public.h"
#undef nullptr
#include "../sys_sdl.h"

namespace {

int freedEventPointers = 0;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "SDL platform test failed: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

void PushEvent(const SDL_Event& event) {
    Require(SDL_PushEvent(const_cast<SDL_Event*>(&event)) == 1, "SDL_PushEvent");
}

void TestKeyboardAndTextEvents() {
    Sys_ClearEvents();
    SDL_Event key{};
    key.type = SDL_KEYDOWN;
    key.key.keysym.scancode = SDL_SCANCODE_A;
    PushEvent(key);

    SDL_Event text{};
    text.type = SDL_TEXTINPUT;
    std::snprintf(text.text.text, sizeof(text.text.text), "A");
    PushEvent(text);

    Sys_PumpEvents();
    const sysEvent_t keyEvent = Sys_GetEvent();
    Require(keyEvent.evType == SE_KEY && keyEvent.evValue == K_A &&
            keyEvent.evValue2 == 1, "keyboard key-down translation");
    const sysEvent_t textEvent = Sys_GetEvent();
    Require(textEvent.evType == SE_CHAR && textEvent.evValue == 'A',
            "text input translation");

    Require(Sys_PollKeyboardInputEvents() == 1, "keyboard polling event count");
    int keyCode = K_NONE;
    bool keyDown = false;
    Require(Sys_ReturnKeyboardInputEvent(0, keyCode, keyDown) == K_A &&
            keyCode == K_A && keyDown, "keyboard polling event value");
    Sys_EndKeyboardInputEvents();
}

void TestMouseEvents() {
    Sys_ClearEvents();
    SDL_Event motion{};
    motion.type = SDL_MOUSEMOTION;
    motion.motion.x = 40;
    motion.motion.y = 24;
    motion.motion.xrel = 5;
    motion.motion.yrel = -3;
    PushEvent(motion);

    SDL_Event button{};
    button.type = SDL_MOUSEBUTTONDOWN;
    button.button.button = SDL_BUTTON_LEFT;
    PushEvent(button);

    SDL_Event wheel{};
    wheel.type = SDL_MOUSEWHEEL;
    wheel.wheel.y = -2;
    PushEvent(wheel);

    Sys_PumpEvents();
    int events[MAX_MOUSE_EVENTS][2]{};
    Require(Sys_PollMouseInputEvents(events) == 4, "mouse polling event count");
    Require(events[0][0] == M_DELTAX && events[0][1] == 5,
            "mouse horizontal motion");
    Require(events[1][0] == M_DELTAY && events[1][1] == -3,
            "mouse vertical motion");
    Require(events[2][0] == M_ACTION1 && events[2][1] == 1,
            "mouse button translation");
    Require(events[3][0] == M_DELTAZ && events[3][1] == -2,
            "mouse wheel translation");

    const sysEvent_t absoluteEvent = Sys_GetEvent();
    Require(absoluteEvent.evType == SE_MOUSE_ABSOLUTE &&
            absoluteEvent.evValue == 40 && absoluteEvent.evValue2 == 24,
            "absolute mouse position");
}

void TestQueueOverflowAndClear() {
    Sys_ClearEvents();
    for (int i = 0; i < 300; ++i) {
        Sys_QueEvent(SE_KEY, i, 1, 0, nullptr, 0);
    }
    Require(Sys_GetEvent().evValue == 44, "queue drops only the oldest events");

    void* payload = std::malloc(8);
    Require(payload != nullptr, "allocate event payload");
    Sys_QueEvent(SE_CONSOLE, 0, 0, 8, payload, 0);
    const int freedBeforeClear = freedEventPointers;
    Sys_ClearEvents();
    Require(freedEventPointers == freedBeforeClear + 1,
            "clear frees pending event payloads");
}

} // namespace

void Mem_Free16(void* ptr) {
    ++freedEventPointers;
    std::free(ptr);
}

int main() {
    SDL_SetMainReady();
    Require(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) == 0,
            "initialize SDL event subsystem");

    TestKeyboardAndTextEvents();
    TestMouseEvents();
    TestQueueOverflowAndClear();

    SDL_Quit();
    std::puts("SDL platform tests passed");
    return EXIT_SUCCESS;
}
