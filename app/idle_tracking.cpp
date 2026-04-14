#include "idle_tracking.h"
#include "settings.h"

#include <cstdlib>

#if defined(TARGET_LINUX)

#include <QObject>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusInterface>

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
#include <wayland-client.h>
#include "wayland/ext-idle-notify-v1-client-protocol.h"

#include <QSocketNotifier>
#include <QElapsedTimer>
#include <QDebug>
#include <algorithm>
#include <cstring>

namespace {

// Idle-tracking backend for Wayland compositors that implement
// ext-idle-notify-v1 (KWin >= 5.26, Mutter >= 45, Sway, Hyprland, ...).
// Registers a single notification with a short timeout and reports elapsed
// time since the compositor marked the seat idle.
class WaylandIdleMonitor
{
public:
    static WaylandIdleMonitor& instance()
    {
        static WaylandIdleMonitor inst;
        return inst;
    }

    // True once the connection and idle-notifier global have been set up.
    // Returns false on compositors that don't implement ext-idle-notify-v1.
    bool available()
    {
        ensure_started();
        return mStarted;
    }

    // Idle time in milliseconds; 0 while the seat is active.
    int get_idle_ms()
    {
        if (!mStarted || !mIdle)
            return 0;
        return static_cast<int>(kTimeoutMs + mIdleSince.elapsed());
    }

private:
    static constexpr uint32_t kTimeoutMs = 1000;

    wl_display* mDisplay = nullptr;
    wl_registry* mRegistry = nullptr;
    wl_seat* mSeat = nullptr;
    ext_idle_notifier_v1* mNotifier = nullptr;
    ext_idle_notification_v1* mNotification = nullptr;
    uint32_t mNotifierVersion = 0;
    QSocketNotifier* mSocket = nullptr;

    bool mStarted = false;
    bool mStartAttempted = false;
    bool mIdle = false;
    QElapsedTimer mIdleSince;

    WaylandIdleMonitor() = default;
    ~WaylandIdleMonitor() { teardown(); }

    WaylandIdleMonitor(const WaylandIdleMonitor&) = delete;
    WaylandIdleMonitor& operator=(const WaylandIdleMonitor&) = delete;

    void ensure_started()
    {
        if (mStarted || mStartAttempted)
            return;
        mStartAttempted = true;

        mDisplay = wl_display_connect(nullptr);
        if (!mDisplay)
            return;

        static const wl_registry_listener reg_listener = {
            &WaylandIdleMonitor::on_global,
            &WaylandIdleMonitor::on_global_remove,
        };
        mRegistry = wl_display_get_registry(mDisplay);
        wl_registry_add_listener(mRegistry, &reg_listener, this);
        wl_display_roundtrip(mDisplay);

        if (!mSeat || !mNotifier)
        {
            qDebug() << "ext-idle-notify-v1 not advertised by compositor";
            teardown();
            return;
        }

        // v2's get_input_idle_notification ignores idle inhibitors, matching
        // the input-only semantics of the X11 and GNOME Mutter backends.
        if (mNotifierVersion >= 2)
            mNotification = ext_idle_notifier_v1_get_input_idle_notification(mNotifier, kTimeoutMs, mSeat);
        else
            mNotification = ext_idle_notifier_v1_get_idle_notification(mNotifier, kTimeoutMs, mSeat);

        static const ext_idle_notification_v1_listener note_listener = {
            &WaylandIdleMonitor::on_idled,
            &WaylandIdleMonitor::on_resumed,
        };
        ext_idle_notification_v1_add_listener(mNotification, &note_listener, this);
        wl_display_flush(mDisplay);

        mSocket = new QSocketNotifier(wl_display_get_fd(mDisplay), QSocketNotifier::Read);
        QObject::connect(mSocket, &QSocketNotifier::activated, mSocket, [this]() {
            if (wl_display_dispatch(mDisplay) < 0)
            {
                qWarning() << "Wayland display dispatch failed; disabling idle monitor";
                mSocket->setEnabled(false);
            }
        });

        mStarted = true;
    }

    void teardown()
    {
        if (mSocket) { delete mSocket; mSocket = nullptr; }
        if (mNotification) { ext_idle_notification_v1_destroy(mNotification); mNotification = nullptr; }
        if (mNotifier) { ext_idle_notifier_v1_destroy(mNotifier); mNotifier = nullptr; }
        if (mSeat) { wl_seat_destroy(mSeat); mSeat = nullptr; }
        if (mRegistry) { wl_registry_destroy(mRegistry); mRegistry = nullptr; }
        if (mDisplay) { wl_display_disconnect(mDisplay); mDisplay = nullptr; }
        mStarted = false;
    }

    static void on_global(void* data, wl_registry* r, uint32_t name, const char* iface, uint32_t version)
    {
        auto self = static_cast<WaylandIdleMonitor*>(data);
        if (std::strcmp(iface, wl_seat_interface.name) == 0 && !self->mSeat)
        {
            self->mSeat = static_cast<wl_seat*>(wl_registry_bind(r, name, &wl_seat_interface, 1));
        }
        else if (std::strcmp(iface, ext_idle_notifier_v1_interface.name) == 0 && !self->mNotifier)
        {
            uint32_t v = std::min(version, 2u);
            self->mNotifier = static_cast<ext_idle_notifier_v1*>(
                wl_registry_bind(r, name, &ext_idle_notifier_v1_interface, v));
            self->mNotifierVersion = v;
        }
    }

    static void on_global_remove(void*, wl_registry*, uint32_t) {}

    static void on_idled(void* data, ext_idle_notification_v1*)
    {
        auto self = static_cast<WaylandIdleMonitor*>(data);
        self->mIdleSince.start();
        self->mIdle = true;
    }

    static void on_resumed(void* data, ext_idle_notification_v1*)
    {
        static_cast<WaylandIdleMonitor*>(data)->mIdle = false;
    }
};

} // namespace

int get_idle_time_wayland()
{
    return WaylandIdleMonitor::instance().get_idle_ms();
}

bool has_wayland_idle_notifier()
{
    return WaylandIdleMonitor::instance().available();
}

#endif

int get_idle_time_dynamically()
{
    const char* wl_display = std::getenv("WAYLAND_DISPLAY");

#if defined(USE_WAYLAND)
    if (wl_display)
    {
        if (has_wayland_idle_notifier())
            return get_idle_time_wayland();
        // Compositor doesn't implement ext-idle-notify-v1 (e.g. older GNOME);
        // fall back to the GNOME Mutter DBus interface.
        return get_idle_time_gnome();
    }
    return get_idle_time_x11();
#else
    if (wl_display)
    {
        static bool warned = false;
        if (!warned) {
            qDebug() << "Wayland is found, but app built for X11 only. Idle tracking is not supported.";
            warned = true;
        }
        return 0;
    }
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
