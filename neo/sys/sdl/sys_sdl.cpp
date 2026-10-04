#include <GL/glew.h>
#include <SDL.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "../../idlib/sys/sys_defines.h"
#include "../../idlib/sys/sys_builddefines.h"
#include "../../idlib/sys/sys_includes.h"
#include "../../idlib/sys/sys_assert.h"
#include "../../idlib/sys/sys_types.h"
#define ID_TIME_T int64
#include "../sys_public.h"
#undef nullptr
#include "sys_sdl.h"

extern void Mem_Free16(void* ptr);

namespace {

constexpr int MAX_QUEUED_EVENTS = 256;
constexpr int EVENT_QUEUE_MASK = MAX_QUEUED_EVENTS - 1;
constexpr int MAX_SDL_CONTROLLERS = 4;

sysEvent_t eventQueue[MAX_QUEUED_EVENTS]{};
int eventHead = 0;
int eventTail = 0;
SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;
bool inputInitialized = false;

struct InputEvent {
    int action;
    int value;
};

struct ControllerSlot {
    SDL_GameController* controller = nullptr;
    SDL_JoystickID instance = -1;
    std::vector<InputEvent> events;
};

std::vector<InputEvent> keyboardEvents;
std::vector<InputEvent> mouseEvents;
std::array<ControllerSlot, MAX_SDL_CONTROLLERS> controllers;
int polledJoystick = -1;

int DoomKeyFromScancode(SDL_Scancode code) {
    switch (code) {
    case SDL_SCANCODE_ESCAPE: return K_ESCAPE;
    case SDL_SCANCODE_1: return K_1;
    case SDL_SCANCODE_2: return K_2;
    case SDL_SCANCODE_3: return K_3;
    case SDL_SCANCODE_4: return K_4;
    case SDL_SCANCODE_5: return K_5;
    case SDL_SCANCODE_6: return K_6;
    case SDL_SCANCODE_7: return K_7;
    case SDL_SCANCODE_8: return K_8;
    case SDL_SCANCODE_9: return K_9;
    case SDL_SCANCODE_0: return K_0;
    case SDL_SCANCODE_MINUS: return K_MINUS;
    case SDL_SCANCODE_EQUALS: return K_EQUALS;
    case SDL_SCANCODE_BACKSPACE: return K_BACKSPACE;
    case SDL_SCANCODE_TAB: return K_TAB;
    case SDL_SCANCODE_Q: return K_Q;
    case SDL_SCANCODE_W: return K_W;
    case SDL_SCANCODE_E: return K_E;
    case SDL_SCANCODE_R: return K_R;
    case SDL_SCANCODE_T: return K_T;
    case SDL_SCANCODE_Y: return K_Y;
    case SDL_SCANCODE_U: return K_U;
    case SDL_SCANCODE_I: return K_I;
    case SDL_SCANCODE_O: return K_O;
    case SDL_SCANCODE_P: return K_P;
    case SDL_SCANCODE_LEFTBRACKET: return K_LBRACKET;
    case SDL_SCANCODE_RIGHTBRACKET: return K_RBRACKET;
    case SDL_SCANCODE_RETURN: return K_ENTER;
    case SDL_SCANCODE_LCTRL: return K_LCTRL;
    case SDL_SCANCODE_A: return K_A;
    case SDL_SCANCODE_S: return K_S;
    case SDL_SCANCODE_D: return K_D;
    case SDL_SCANCODE_F: return K_F;
    case SDL_SCANCODE_G: return K_G;
    case SDL_SCANCODE_H: return K_H;
    case SDL_SCANCODE_J: return K_J;
    case SDL_SCANCODE_K: return K_K;
    case SDL_SCANCODE_L: return K_L;
    case SDL_SCANCODE_SEMICOLON: return K_SEMICOLON;
    case SDL_SCANCODE_APOSTROPHE: return K_APOSTROPHE;
    case SDL_SCANCODE_GRAVE: return K_GRAVE;
    case SDL_SCANCODE_LSHIFT: return K_LSHIFT;
    case SDL_SCANCODE_BACKSLASH: return K_BACKSLASH;
    case SDL_SCANCODE_Z: return K_Z;
    case SDL_SCANCODE_X: return K_X;
    case SDL_SCANCODE_C: return K_C;
    case SDL_SCANCODE_V: return K_V;
    case SDL_SCANCODE_B: return K_B;
    case SDL_SCANCODE_N: return K_N;
    case SDL_SCANCODE_M: return K_M;
    case SDL_SCANCODE_COMMA: return K_COMMA;
    case SDL_SCANCODE_PERIOD: return K_PERIOD;
    case SDL_SCANCODE_SLASH: return K_SLASH;
    case SDL_SCANCODE_RSHIFT: return K_RSHIFT;
    case SDL_SCANCODE_KP_MULTIPLY: return K_KP_STAR;
    case SDL_SCANCODE_LALT: return K_LALT;
    case SDL_SCANCODE_SPACE: return K_SPACE;
    case SDL_SCANCODE_CAPSLOCK: return K_CAPSLOCK;
    case SDL_SCANCODE_F1: return K_F1;
    case SDL_SCANCODE_F2: return K_F2;
    case SDL_SCANCODE_F3: return K_F3;
    case SDL_SCANCODE_F4: return K_F4;
    case SDL_SCANCODE_F5: return K_F5;
    case SDL_SCANCODE_F6: return K_F6;
    case SDL_SCANCODE_F7: return K_F7;
    case SDL_SCANCODE_F8: return K_F8;
    case SDL_SCANCODE_F9: return K_F9;
    case SDL_SCANCODE_F10: return K_F10;
    case SDL_SCANCODE_NUMLOCKCLEAR: return K_NUMLOCK;
    case SDL_SCANCODE_SCROLLLOCK: return K_SCROLL;
    case SDL_SCANCODE_KP_7: return K_KP_7;
    case SDL_SCANCODE_KP_8: return K_KP_8;
    case SDL_SCANCODE_KP_9: return K_KP_9;
    case SDL_SCANCODE_KP_MINUS: return K_KP_MINUS;
    case SDL_SCANCODE_KP_4: return K_KP_4;
    case SDL_SCANCODE_KP_5: return K_KP_5;
    case SDL_SCANCODE_KP_6: return K_KP_6;
    case SDL_SCANCODE_KP_PLUS: return K_KP_PLUS;
    case SDL_SCANCODE_KP_1: return K_KP_1;
    case SDL_SCANCODE_KP_2: return K_KP_2;
    case SDL_SCANCODE_KP_3: return K_KP_3;
    case SDL_SCANCODE_KP_0: return K_KP_0;
    case SDL_SCANCODE_KP_PERIOD: return K_KP_DOT;
    case SDL_SCANCODE_F11: return K_F11;
    case SDL_SCANCODE_F12: return K_F12;
    case SDL_SCANCODE_KP_ENTER: return K_KP_ENTER;
    case SDL_SCANCODE_RCTRL: return K_RCTRL;
    case SDL_SCANCODE_KP_COMMA: return K_KP_COMMA;
    case SDL_SCANCODE_KP_DIVIDE: return K_KP_SLASH;
    case SDL_SCANCODE_PRINTSCREEN: return K_PRINTSCREEN;
    case SDL_SCANCODE_RALT: return K_RALT;
    case SDL_SCANCODE_PAUSE: return K_PAUSE;
    case SDL_SCANCODE_HOME: return K_HOME;
    case SDL_SCANCODE_UP: return K_UPARROW;
    case SDL_SCANCODE_PAGEUP: return K_PGUP;
    case SDL_SCANCODE_LEFT: return K_LEFTARROW;
    case SDL_SCANCODE_RIGHT: return K_RIGHTARROW;
    case SDL_SCANCODE_END: return K_END;
    case SDL_SCANCODE_DOWN: return K_DOWNARROW;
    case SDL_SCANCODE_PAGEDOWN: return K_PGDN;
    case SDL_SCANCODE_INSERT: return K_INS;
    case SDL_SCANCODE_DELETE: return K_DEL;
    case SDL_SCANCODE_LGUI: return K_LWIN;
    case SDL_SCANCODE_RGUI: return K_RWIN;
    case SDL_SCANCODE_APPLICATION: return K_APPS;
    default: return K_NONE;
    }
}

int DoomMouseButton(Uint8 button) {
    switch (button) {
    case SDL_BUTTON_LEFT: return K_MOUSE1;
    case SDL_BUTTON_RIGHT: return K_MOUSE2;
    case SDL_BUTTON_MIDDLE: return K_MOUSE3;
    case SDL_BUTTON_X1: return K_MOUSE4;
    case SDL_BUTTON_X2: return K_MOUSE5;
    default: return K_NONE;
    }
}

int DecodeUtf8(const char*& text) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(text);
    int codepoint = 0;
    int length = 0;

    if (bytes[0] < 0x80) {
        codepoint = bytes[0];
        length = 1;
    } else if ((bytes[0] & 0xE0) == 0xC0) {
        codepoint = bytes[0] & 0x1F;
        length = 2;
    } else if ((bytes[0] & 0xF0) == 0xE0) {
        codepoint = bytes[0] & 0x0F;
        length = 3;
    } else if ((bytes[0] & 0xF8) == 0xF0) {
        codepoint = bytes[0] & 0x07;
        length = 4;
    } else {
        ++text;
        return 0xFFFD;
    }

    for (int i = 1; i < length; ++i) {
        if ((bytes[i] & 0xC0) != 0x80) {
            ++text;
            return 0xFFFD;
        }
        codepoint = (codepoint << 6) | (bytes[i] & 0x3F);
    }
    text += length;
    return codepoint;
}

int ControllerSlotForInstance(SDL_JoystickID instance) {
    for (int i = 0; i < MAX_SDL_CONTROLLERS; ++i) {
        if (controllers[i].instance == instance) {
            return i;
        }
    }
    return -1;
}

void QueueJoystickEvent(int device, int action, int value) {
    if (device < 0 || device >= MAX_SDL_CONTROLLERS) {
        return;
    }
    controllers[device].events.push_back({ action, value });
    if (action >= J_AXIS_MIN && action <= J_AXIS_MAX) {
        const int axis = action - J_AXIS_MIN;
        const int percent = (value * 16) / 32767;
        Sys_QueEvent(SE_JOYSTICK, axis, percent, 0, nullptr, device);
    } else if (action >= J_ACTION1 && action <= J_ACTION_MAX) {
        Sys_QueEvent(SE_KEY, K_JOY1 + action - J_ACTION1, value != 0, 0, nullptr, device);
    } else if (action >= J_DPAD_UP && action <= J_DPAD_RIGHT) {
        Sys_QueEvent(SE_KEY, K_JOY_DPAD_UP + action - J_DPAD_UP,
                     value != 0, 0, nullptr, device);
    }
}

void QueueControllerAxis(int device, SDL_GameControllerAxis axis, Sint16 value) {
    int action;
    int scaled;
    switch (axis) {
    case SDL_CONTROLLER_AXIS_LEFTX:
        action = J_AXIS_LEFT_X;
        scaled = value;
        break;
    case SDL_CONTROLLER_AXIS_LEFTY:
        action = J_AXIS_LEFT_Y;
        scaled = value;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTX:
        action = J_AXIS_RIGHT_X;
        scaled = value;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTY:
        action = J_AXIS_RIGHT_Y;
        scaled = value;
        break;
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        action = J_AXIS_LEFT_TRIG;
        scaled = (static_cast<int>(value) + 32768) / 2;
        break;
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
        action = J_AXIS_RIGHT_TRIG;
        scaled = (static_cast<int>(value) + 32768) / 2;
        break;
    default:
        return;
    }

    QueueJoystickEvent(device, action, scaled);
}

} // namespace

void Sys_Init() {
    if (!Sys_SDL_Init()) {
        Sys_Error("SDL initialization failed: %s", SDL_GetError());
    }
}

void Sys_Shutdown() {
    Sys_ClearEvents();
    Sys_ShutdownInput();
    Sys_SDL_DestroyWindowAndContext();
}

void Sys_Error(const char* error, ...) {
    char message[2048];
    va_list args;
    va_start(args, error);
    std::vsnprintf(message, sizeof(message), error, args);
    va_end(args);
    message[sizeof(message) - 1] = '\0';

    std::fprintf(stderr, "Fatal error: %s\n", message);
    if ((SDL_WasInit(0) & SDL_INIT_VIDEO) != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Doom 3 BFG", message,
                                 window);
    }
    Sys_SDL_DestroyWindowAndContext();
    std::exit(EXIT_FAILURE);
}

void Sys_Quit() {
    Sys_ClearEvents();
    Sys_ShutdownInput();
    Sys_SDL_DestroyWindowAndContext();
    std::exit(EXIT_SUCCESS);
}

void Sys_Sleep(int msec) {
    SDL_Delay(static_cast<Uint32>(msec < 0 ? 0 : msec));
}

void Sys_Yield() {
    SDL_Delay(0);
}

int Sys_Milliseconds() {
    static const Uint64 start = SDL_GetPerformanceCounter();
    const Uint64 elapsed = SDL_GetPerformanceCounter() - start;
    return static_cast<int>(
        static_cast<double>(elapsed) * 1000.0 /
        static_cast<double>(SDL_GetPerformanceFrequency()));
}

uint64 Sys_Microseconds() {
    static const Uint64 start = SDL_GetPerformanceCounter();
    const Uint64 elapsed = SDL_GetPerformanceCounter() - start;
    return static_cast<uint64>(
        static_cast<double>(elapsed) * 1000000.0 /
        static_cast<double>(SDL_GetPerformanceFrequency()));
}

double Sys_GetClockTicks() {
    return static_cast<double>(SDL_GetPerformanceCounter());
}

double Sys_ClockTicksPerSecond() {
    return static_cast<double>(SDL_GetPerformanceFrequency());
}

cpuid_t Sys_GetProcessorId() {
#if defined(_M_X64) || defined(__x86_64__)
    return static_cast<cpuid_t>(CPUID_GENERIC | CPUID_MMX | CPUID_SSE |
                                CPUID_SSE2);
#else
    return CPUID_GENERIC;
#endif
}

const char* Sys_GetProcessorString() {
#if defined(_M_X64) || defined(__x86_64__)
    return "x86-64 with SSE2";
#else
    return "generic processor";
#endif
}

int Sys_GetSystemRam() {
    return SDL_GetSystemRAM();
}

int Sys_GetVideoRam() {
    return 64;
}

void Sys_QueEvent(sysEventType_t type, int value, int value2,
                  int ptrLength, void* ptr, int inputDeviceNum) {
    if (eventHead - eventTail >= MAX_QUEUED_EVENTS) {
        std::fprintf(stderr, "Sys_QueEvent: event queue overflow\n");
        sysEvent_t& dropped = eventQueue[eventTail & EVENT_QUEUE_MASK];
        if (dropped.evPtr != nullptr) {
            Mem_Free16(dropped.evPtr);
        }
        ++eventTail;
    }

    sysEvent_t& event = eventQueue[eventHead & EVENT_QUEUE_MASK];
    event.evType = type;
    event.evValue = value;
    event.evValue2 = value2;
    event.evPtrLength = ptrLength;
    event.evPtr = ptr;
    event.inputDevice = inputDeviceNum;
    ++eventHead;
}

void Sys_PumpEvents() {
    keyboardEvents.clear();
    mouseEvents.clear();
    for (ControllerSlot& controller : controllers) {
        controller.events.clear();
    }

    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            Sys_Quit();
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            const int key = DoomKeyFromScancode(event.key.keysym.scancode);
            if (key != K_NONE) {
                const bool down = event.type == SDL_KEYDOWN;
                keyboardEvents.push_back({ key, down ? 1 : 0 });
                const bool pollingKey = key == K_PRINTSCREEN || key == K_LCTRL ||
                    key == K_LALT || key == K_RCTRL || key == K_RALT;
                if (!pollingKey) {
                    Sys_QueEvent(SE_KEY, key, down, 0, nullptr, 0);
                }
            }
            break;
        }
        case SDL_TEXTINPUT: {
            const char* text = event.text.text;
            const char* const end = text + std::strlen(text);
            while (text < end) {
                Sys_QueEvent(SE_CHAR, DecodeUtf8(text), 0, 0, nullptr, 0);
            }
            break;
        }
        case SDL_MOUSEMOTION:
            Sys_QueEvent(SE_MOUSE_ABSOLUTE, event.motion.x, event.motion.y, 0, nullptr, 0);
            if (event.motion.xrel != 0) {
                mouseEvents.push_back({ M_DELTAX, event.motion.xrel });
            }
            if (event.motion.yrel != 0) {
                mouseEvents.push_back({ M_DELTAY, event.motion.yrel });
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            const int key = DoomMouseButton(event.button.button);
            if (key != K_NONE) {
                const bool down = event.type == SDL_MOUSEBUTTONDOWN;
                const int button = key - K_MOUSE1;
                if (button < 8) {
                    mouseEvents.push_back({ M_ACTION1 + button, down ? 1 : 0 });
                }
                Sys_QueEvent(SE_KEY, key, down, 0, nullptr, 0);
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            const int direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
            const int vertical = direction * event.wheel.y;
            const int horizontal = direction * event.wheel.x;
            if (vertical != 0) {
                mouseEvents.push_back({ M_DELTAZ, vertical });
            }
            if (horizontal != 0) {
                mouseEvents.push_back({ M_DELTAX, horizontal });
            }
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            const int slot = ControllerSlotForInstance(event.cbutton.which);
            if (slot < 0) {
                break;
            }
            int action = -1;
            switch (event.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_A: action = J_ACTION1; break;
            case SDL_CONTROLLER_BUTTON_B: action = J_ACTION2; break;
            case SDL_CONTROLLER_BUTTON_X: action = J_ACTION3; break;
            case SDL_CONTROLLER_BUTTON_Y: action = J_ACTION4; break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: action = J_ACTION5; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: action = J_ACTION6; break;
            case SDL_CONTROLLER_BUTTON_BACK: action = J_ACTION7; break;
            case SDL_CONTROLLER_BUTTON_START: action = J_ACTION8; break;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: action = J_ACTION9; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: action = J_ACTION10; break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP: action = J_DPAD_UP; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: action = J_DPAD_DOWN; break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: action = J_DPAD_LEFT; break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: action = J_DPAD_RIGHT; break;
            default: break;
            }
            if (action >= 0) {
                QueueJoystickEvent(slot, action,
                                   event.type == SDL_CONTROLLERBUTTONDOWN ? 1 : 0);
            }
            break;
        }
        case SDL_CONTROLLERAXISMOTION:
            QueueControllerAxis(ControllerSlotForInstance(event.caxis.which),
                                static_cast<SDL_GameControllerAxis>(event.caxis.axis),
                                event.caxis.value);
            break;
        case SDL_CONTROLLERDEVICEADDED:
            if (SDL_IsGameController(event.cdevice.which)) {
                SDL_GameController* controller = SDL_GameControllerOpen(event.cdevice.which);
                if (controller == nullptr) {
                    std::fprintf(stderr, "SDL_GameControllerOpen failed: %s\n", SDL_GetError());
                } else {
                    const SDL_JoystickID instance =
                        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
                    const auto slot = std::find_if(
                        controllers.begin(), controllers.end(),
                        [](const ControllerSlot& item) { return item.controller == nullptr; }
                    );
                    if (slot == controllers.end()) {
                        SDL_GameControllerClose(controller);
                        std::fprintf(stderr, "SDL controller limit reached\n");
                    } else {
                        slot->controller = controller;
                        slot->instance = instance;
                    }
                }
            }
            break;
        case SDL_CONTROLLERDEVICEREMOVED: {
            for (ControllerSlot& slot : controllers) {
                if (slot.instance == event.cdevice.which) {
                    SDL_GameControllerClose(slot.controller);
                    slot.controller = nullptr;
                    slot.instance = -1;
                    slot.events.clear();
                    break;
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

sysEvent_t Sys_GetEvent() {
    if (eventHead > eventTail) {
        return eventQueue[eventTail++ & EVENT_QUEUE_MASK];
    }

    sysEvent_t event{};
    return event;
}

int Sys_PollKeyboardInputEvents() {
    return static_cast<int>(keyboardEvents.size());
}

int Sys_ReturnKeyboardInputEvent(const int n, int& ch, bool& state) {
    if (n < 0 || static_cast<size_t>(n) >= keyboardEvents.size()) {
        return 0;
    }
    ch = keyboardEvents[n].action;
    state = keyboardEvents[n].value != 0;
    return ch;
}

void Sys_EndKeyboardInputEvents() {
}

int Sys_PollMouseInputEvents(int output[MAX_MOUSE_EVENTS][2]) {
    const int count = static_cast<int>(mouseEvents.size() < MAX_MOUSE_EVENTS
        ? mouseEvents.size() : MAX_MOUSE_EVENTS);
    for (int i = 0; i < count; ++i) {
        output[i][0] = mouseEvents[i].action;
        output[i][1] = mouseEvents[i].value;
        if (mouseEvents[i].action == M_DELTAX) {
            Sys_QueEvent(SE_MOUSE, mouseEvents[i].value, 0, 0, nullptr, 0);
        } else if (mouseEvents[i].action == M_DELTAY) {
            Sys_QueEvent(SE_MOUSE, 0, mouseEvents[i].value, 0, nullptr, 0);
        } else if (mouseEvents[i].action == M_DELTAZ) {
            const int key = mouseEvents[i].value < 0 ? K_MWHEELDOWN : K_MWHEELUP;
            for (int click = 0; click < SDL_abs(mouseEvents[i].value); ++click) {
                Sys_QueEvent(SE_KEY, key, 1, 0, nullptr, 0);
                Sys_QueEvent(SE_KEY, key, 0, 0, nullptr, 0);
            }
        }
    }
    return count;
}

void Sys_SetRumble(int device, int low, int hi) {
    if (device < 0 || device >= MAX_SDL_CONTROLLERS ||
        controllers[device].controller == nullptr) {
        return;
    }

    const Uint16 lowMotor = static_cast<Uint16>(low < 0 ? 0 : low > 65535 ? 65535 : low);
    const Uint16 highMotor = static_cast<Uint16>(hi < 0 ? 0 : hi > 65535 ? 65535 : hi);
    if (SDL_GameControllerRumble(controllers[device].controller, lowMotor, highMotor,
                                 0xFFFFFFFFu) != 0) {
        std::fprintf(stderr, "SDL_GameControllerRumble failed: %s\n", SDL_GetError());
    }
}

int Sys_PollJoystickInputEvents(int deviceNum) {
    polledJoystick = deviceNum >= 0 && deviceNum < MAX_SDL_CONTROLLERS &&
                     controllers[deviceNum].controller != nullptr ? deviceNum : -1;
    return polledJoystick < 0
        ? 0 : static_cast<int>(controllers[polledJoystick].events.size());
}

int Sys_ReturnJoystickInputEvent(const int n, int& action, int& value) {
    if (polledJoystick < 0 ||
        static_cast<size_t>(n) >= controllers[polledJoystick].events.size()) {
        return 0;
    }
    action = controllers[polledJoystick].events[n].action;
    value = controllers[polledJoystick].events[n].value;
    return 1;
}

void Sys_EndJoystickInputEvents() {
    polledJoystick = -1;
}

void Sys_GenerateEvents() {
    Sys_PumpEvents();
}

void Sys_ClearEvents() {
    while (eventTail < eventHead) {
        sysEvent_t& event = eventQueue[eventTail++ & EVENT_QUEUE_MASK];
        if (event.evPtr != nullptr) {
            Mem_Free16(event.evPtr);
            event.evPtr = nullptr;
        }
    }
    eventHead = eventTail = 0;
}

void Sys_InitInput() {
    inputInitialized = true;
    if (window != nullptr) {
        SDL_StartTextInput();
    }
}

void Sys_ShutdownInput() {
    inputInitialized = false;
    SDL_StopTextInput();
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

void Sys_GrabMouseCursor(bool grabIt) {
    if (SDL_SetRelativeMouseMode(grabIt ? SDL_TRUE : SDL_FALSE) != 0) {
        std::fprintf(stderr, "SDL_SetRelativeMouseMode failed: %s\n", SDL_GetError());
    }
}

void Sys_ShowWindow(bool show) {
    if (window == nullptr) {
        return;
    }
    if (show) {
        SDL_ShowWindow(window);
    } else {
        SDL_HideWindow(window);
    }
}

bool Sys_IsWindowVisible() {
    return window != nullptr && (SDL_GetWindowFlags(window) & SDL_WINDOW_SHOWN) != 0;
}

bool Sys_SDL_Init() {
    const Uint32 required = SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS |
                            SDL_INIT_GAMECONTROLLER;
    const Uint32 initialized = SDL_WasInit(required);
    const Uint32 missing = required & ~initialized;
    if (missing == 0) {
        return true;
    }

    const int result = initialized == 0
        ? SDL_Init(required)
        : SDL_InitSubSystem(missing);
    if (result != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool Sys_SDL_CreateWindowAndContext(const char* title, int width, int height,
                                    int fullscreen, int multiSamples) {
    if (!Sys_SDL_Init()) {
        return false;
    }

    const SDL_GLattr attributes[] = {
        SDL_GL_CONTEXT_MAJOR_VERSION,
        SDL_GL_CONTEXT_MINOR_VERSION,
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_DOUBLEBUFFER,
        SDL_GL_DEPTH_SIZE,
        SDL_GL_STENCIL_SIZE,
        SDL_GL_MULTISAMPLEBUFFERS,
        SDL_GL_MULTISAMPLESAMPLES
    };
#if defined(DOOM3BFG_GL_CORE_PROFILE)
    const int profile = SDL_GL_CONTEXT_PROFILE_CORE;
#else
    const int profile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
#endif
    const int values[] = {
        3, 2, profile, 1, 24, 8,
        multiSamples > 0 ? 1 : 0,
        multiSamples > 0 ? multiSamples : 0
    };
    for (int i = 0; i < static_cast<int>(sizeof(attributes) / sizeof(attributes[0])); ++i) {
        if (SDL_GL_SetAttribute(attributes[i], values[i]) != 0) {
            std::fprintf(stderr, "SDL_GL_SetAttribute failed: %s\n", SDL_GetError());
            SDL_Quit();
            return false;
        }
    }

    window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
            (fullscreen < 0 ? SDL_WINDOW_BORDERLESS : 0)
    );
    if (window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    if (fullscreen != 0 && !Sys_SDL_SetScreenMode(0, 0, width, height, fullscreen)) {
        Sys_SDL_DestroyWindowAndContext();
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        window = nullptr;
        SDL_Quit();
        return false;
    }

    glewExperimental = GL_TRUE;
    const GLenum result = glewInit();
    if (result != GLEW_OK) {
        std::fprintf(stderr, "glewInit failed: %s\n",
                     reinterpret_cast<const char*>(glewGetErrorString(result)));
        Sys_SDL_DestroyWindowAndContext();
        return false;
    }

    if (inputInitialized) {
        SDL_StartTextInput();
    }
    return true;
}

bool Sys_SDL_SetScreenMode(int x, int y, int width, int height, int fullscreen) {
    if (window == nullptr) {
        std::fprintf(stderr, "Cannot change video mode before creating the SDL window\n");
        return false;
    }

    Uint32 flags = 0;
    if (fullscreen > 0) {
        const int display = fullscreen - 1;
        if (display >= SDL_GetNumVideoDisplays()) {
            std::fprintf(stderr, "SDL display index %d is unavailable\n", display);
            return false;
        }
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(display),
                              SDL_WINDOWPOS_CENTERED_DISPLAY(display));
        flags = SDL_WINDOW_FULLSCREEN;
    } else if (fullscreen < 0) {
        SDL_SetWindowBordered(window, SDL_FALSE);
        flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
    } else {
        SDL_SetWindowBordered(window, SDL_TRUE);
        SDL_SetWindowSize(window, width, height);
        SDL_SetWindowPosition(window, x, y);
    }

    if (SDL_SetWindowFullscreen(window, flags) != 0) {
        std::fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

void Sys_SDL_DestroyWindowAndContext() {
    SDL_StopTextInput();
    for (ControllerSlot& slot : controllers) {
        if (slot.controller != nullptr) {
            SDL_GameControllerClose(slot.controller);
            slot.controller = nullptr;
            slot.instance = -1;
        }
        slot.events.clear();
    }
    if (glContext != nullptr) {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void Sys_SDL_SwapBuffers() {
    if (window != nullptr) {
        SDL_GL_SwapWindow(window);
    }
}

bool Sys_SDL_MakeContextCurrent(bool current) {
    if (window == nullptr) {
        return !current;
    }
    if (SDL_GL_MakeCurrent(window, current ? glContext : nullptr) != 0) {
        std::fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool Sys_SDL_SetGammaRamp(const Uint16 red[256], const Uint16 green[256],
                          const Uint16 blue[256]) {
    if (window == nullptr) {
        return false;
    }
    if (SDL_SetWindowGammaRamp(window, red, green, blue) != 0) {
        std::fprintf(stderr, "SDL_SetWindowGammaRamp failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool Sys_SDL_SetVSync(int interval) {
    if (glContext == nullptr) {
        std::fprintf(stderr, "Cannot set VSync before creating the SDL OpenGL context\n");
        return false;
    }
    if (SDL_GL_SetSwapInterval(interval) != 0) {
        std::fprintf(stderr, "SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}
