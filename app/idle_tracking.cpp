#include "idle_tracking.h"
#include "settings.h"

#include <cstdlib>

#if defined(TARGET_LINUX)

#include <QObject>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusInterface>

// Thanks to https://stackoverflow.com/questions/222606/detecting-keyboard-mouse-activity-in-linux

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// This requires sudo apt install libxss-dev
#include <X11/extensions/scrnsaver.h> // This can require libxss-dev to be installed
#include <dlfcn.h>
// #include <qmetatype.h>
// #include <QDBusConnection>

/*
// Prototype from stackoverflow
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
*/

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

int get_idle_time_gnome()
{
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return 0;

    QDBusInterface interface( "org.gnome.Mutter.IdleMonitor",
                              "/org/gnome/Mutter/IdleMonitor/Core",
                              "org.gnome.Mutter.IdleMonitor");

    QDBusReply<int> reply = interface.call("GetIdletime");

    return reply.isValid() ? reply.value() : 0;
}

#if defined(USE_WAYLAND)
#include <wayland-client-protocol-unstable.hpp>

class kde_idle_detector
{
private:
    wayland::seat_t seat;
    wayland::display_t d;
    wayland::org_kde_kwin_idle_t idle;
    wayland::org_kde_kwin_idle_timeout_t idle_timer;

    uint64_t idle_start = 0;
    uint64_t idle_finish = 0;
    bool active = false;

public:
    kde_idle_detector()
    {}
    ~kde_idle_detector()
    {
        stop();
    }

    // Idle timeout is in msec
    void start(int idle_timeout)
    {
        if (active)
            return;

        auto registry = d.get_registry();
        registry.on_global() = [&] (uint32_t name, const std::string& interface, uint32_t version)
        {
          if (interface == wayland::seat_t::interface_name)
            registry.bind(name, this->seat, version);
          else
          if (interface == wayland::org_kde_kwin_idle_t::interface_name)
              registry.bind(name, this->idle, version);
        };
        d.roundtrip();


        bool has_keyboard = false, has_pointer = false;
        seat.on_capabilities() = [&] (const wayland::seat_capability& capability)
        {
          has_keyboard = capability & wayland::seat_capability::keyboard;
          has_pointer = capability & wayland::seat_capability::pointer;
        };
        d.roundtrip();

        idle_timer = idle.get_idle_timeout(seat, idle_timeout);
        idle_timer.on_idle() = [&]()
        {
            idle_start = ::time(nullptr);
        };

        idle_timer.on_resumed() = [&]()
        {
            idle_finish = ::time(nullptr);
        };

        active = true;
    }

    void stop()
    {
        if (!active)
            return;

        active = false;
        idle_timer.release();
        seat.release();
    }

    // Return idle time in microseconds
    int get_idle_time() const
    {
        if (idle_start > idle_finish)
        {
            return (::time(nullptr) - idle_start) * 1000;
        }

        return 0;
    }
};

kde_idle_detector kde_idle;
int get_idle_time_kde_wayland()
{
    // Ensure idle detector runs
    kde_idle.start(1);

    return kde_idle.get_idle_time();
}

#endif

int get_idle_time_dynamically()
{
    const char* wl_display = std::getenv("WAYLAND_DISPLAY");
    // const char* x11_display = std::getenv("DISPLAY");

#if defined(USE_WAYLAND)
    if (wl_display)
    {
        const char* desktop_name = std::getenv("XDG_SESSION_DESKTOP");
        if (!desktop_name)
            return 0;

        if (strcmp(desktop_name, "KDE") == 0)
            return get_idle_time_kde_wayland();
        else
        if (strcmp(desktop_name, "GNOME") == 0)
            return get_idle_time_gnome();
        else
            return 0;
    }
    else
        return get_idle_time_x11();
#else
    // Restrict to X11
    if (wl_display)
        return 0;

    return get_idle_time_x11();
#endif
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
