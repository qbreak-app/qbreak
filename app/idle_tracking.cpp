#include "idle_tracking.h"
#include "settings.h"

#include <cstdlib>

#if defined(TARGET_LINUX)

// Thanks to https://stackoverflow.com/questions/222606/detecting-keyboard-mouse-activity-in-linux

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h> // This can require libxss-dev to be installed
#include <dlfcn.h>

int get_idle_time()
{
        time_t idle_time;
        static XScreenSaverInfo *mit_info;
        Display *display;
        int screen;

        mit_info = XScreenSaverAllocInfo();
        if ((display = XOpenDisplay(NULL)) == NULL) {
            return -1;
        }

        screen = DefaultScreen(display);
        XScreenSaverQueryInfo(display, RootWindow(display, screen), mit_info);
        idle_time = (mit_info->idle);
        XFree(mit_info);
        XCloseDisplay(display);

        return idle_time;
}

int get_idle_time_x11()
{
    void* lib_xss = dlopen("libXss.so", RTLD_LAZY);
    if (!lib_xss)
        return 0;

    void* lib_x11 = dlopen("libX11.so", RTLD_LAZY);
    if (!lib_x11)
        return 0;

    typedef XScreenSaverInfo* (*xss_alloc_info)(void);
    xss_alloc_info alloc_info = (xss_alloc_info)dlsym(lib_xss, "XScreenSaverAllocInfo");

    typedef Display* (*x11_open_display)(void*);
    x11_open_display open_display = (x11_open_display)dlsym(lib_x11, "XOpenDisplay");


    typedef Status (*xss_query_info)(    Display*		/* display */,
                                         Drawable		/* drawable */,
                                         XScreenSaverInfo*	/* info */);
    xss_query_info query_info = (xss_query_info)dlsym(lib_xss, "XScreenSaverQueryInfo");

    typedef int (*x11_free)(void*);
    x11_free free_mem = (x11_free)dlsym(lib_x11, "XFree");

    typedef int (*x11_close_display)(Display* display);
    x11_close_display close_display = (x11_close_display)dlsym(lib_x11, "XCloseDisplay");


    time_t idle_time;
    static XScreenSaverInfo *mit_info;
    Display *display;
    int screen;

    mit_info = alloc_info();
    if ((display = open_display(NULL)) == NULL) {
        return -1;
    }

    screen = DefaultScreen(display);
    query_info(display, RootWindow(display, screen), mit_info);
    idle_time = (mit_info->idle);
    free_mem(mit_info);
    close_display(display);

    dlclose(lib_xss);
    dlclose(lib_x11);
    return idle_time;
}

int get_idle_time_wayland()
{
    // Some ideas:
    // https://dev.gajim.org/gajim/gajim/-/commit/2e5d966f1d715f20f112dd9370f6ccd13fcaeca9
    // https://gitlab.freedesktop.org/wayland/wayland-protocols/-/merge_requests/29
    // https://github.com/NilsBrause/waylandpp

    return 0;
}

int get_idle_time_dynamically()
{
    // Check if we run under X11
    // const char* x11_display = std::getenv("DISPLAY");

    // Check if we run under Wayland
    // https://unix.stackexchange.com/questions/202891/how-to-know-whether-wayland-or-x11-is-being-used/371164#371164
    const char* wl_display = std::getenv("WAYLAND_DISPLAY");
    if (wl_display)
        return get_idle_time_wayland();
    else
        return get_idle_time_x11();
}

#endif

#if defined(TARGET_WINDOWS)

// To handle Windows case later
// https://stackoverflow.com/questions/8820615/how-to-check-in-c-if-the-system-is-active

/*
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>

// do something after 10 minutes of user inactivity
static const unsigned int idle_milliseconds = 60*10*1000;
// wait at least an hour between two runs
static const unsigned int interval = 60*60*1000;

int main() {
    LASTINPUTINFO last_input;
    BOOL screensaver_active;

    // main loop to check if user has been idle long enough
    for (;;) {
        if ( !GetLastInputInfo(&last_input)
          || !SystemParametersInfo(SPI_GETSCREENSAVERACTIVE, 0,
                                   &screensaver_active, 0))
        {
            std::cerr << "WinAPI failed!" << std::endl;
            return ERROR_FAILURE;
        }

        if (last_input.dwTime < idle_milliseconds && !screensaver_active) {
            // user hasn't been idle for long enough
            // AND no screensaver is running
            Sleep(1000);
            continue;
        }

        // user has been idle at least 10 minutes
        do_something();
        // done. Wait before doing the next loop.
        Sleep(interval);
    }
}
*/
#endif
