#include "idle_tracking.h"
#include "settings.h"
#if defined(TARGET_LINUX)

// Thanks to https://stackoverflow.com/questions/222606/detecting-keyboard-mouse-activity-in-linux

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>


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
        idle_time = (mit_info->idle) / 1000 / 60;
        XFree(mit_info);
        XCloseDisplay(display);

        return idle_time;
}


#endif
