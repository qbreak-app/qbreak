#ifndef AUTOSTART_H
#define AUTOSTART_H

#include <string>

class autostart
{
public:
    static void enable(const std::string& path_to_me);
    static void disable();
    static bool is_enabled();
};

class appmenu
{
public:
    static void install(const std::string& path_to_me);
    static void uninstall();
    static bool is_installed();
};

#endif // AUTOSTART_H
