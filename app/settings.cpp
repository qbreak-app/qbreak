#include "settings.h"

#include <QSettings>

// Key names for settings
const QString Key_LongBreak_Interval            = "LongBreak_Interval";
const QString Key_LongBreak_PostponeInterval    = "LongBreak_Postpone_Interval";
const QString Key_LongBreak_Length              = "LongBreak_Length";
const QString Key_Window_On_Top                 = "Window_On_Top";
const QString Key_Verbose                       = "Verbose";
const QString Key_Autostart                     = "Autostart";

void app_settings::save(const config &cfg)
{
    QSettings s;

    s.setValue(Key_LongBreak_Interval,          cfg.longbreak_interval);
    s.setValue(Key_LongBreak_Length,            cfg.longbreak_length);
    s.setValue(Key_LongBreak_PostponeInterval,  cfg.longbreak_postpone_interval);
    s.setValue(Key_Window_On_Top,               cfg.window_on_top);
    s.setValue(Key_Verbose,                     cfg.verbose);
    s.setValue(Key_Autostart,                   cfg.autostart);
}

app_settings::config app_settings::load()
{
    QSettings s;
    config r;

    r.longbreak_interval            = s.value(Key_LongBreak_Interval,           Default_LongBreak_Interval).toInt();
    r.longbreak_length              = s.value(Key_LongBreak_Length,             Default_LongBreak_Length).toInt();
    r.longbreak_postpone_interval   = s.value(Key_LongBreak_PostponeInterval,   Default_LongBreak_PostponeInterval).toInt();
    r.window_on_top                 = s.value(Key_Window_On_Top,                Default_Autostart).toBool();
    r.verbose                       = s.value(Key_Verbose,                      Default_Verbose).toBool();
    r.autostart                     = s.value(Key_Autostart,                    Default_Autostart).toBool();
    return r;
}
