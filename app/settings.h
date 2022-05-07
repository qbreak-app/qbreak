#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <map>

// Default values in seconds
const int Default_LongBreak_Interval = 50 * 60;
const int Default_LongBreak_PostponeInterval = 10 * 60;
const int Default_LongBreak_Length = 10 * 60;
const int Default_Notify_Length = 30;

// Is break window is running on the top ?
const bool Default_WindowOnTop = true;

// Is verbose debug output
const bool Default_Verbose = false;

// Default autostart
const bool Default_Autostart = true;

const QString Default_Monitor = "";
const QString Primary_Monitor = "[Primary]";

// Default behavior is not play any audio on break end.
const QString Audio_Empty = "None";
const QString Audio_Default_1 = "Default 1";
const QString Audio_Default_2 = "Default 2";
const QString Audio_Custom = "Custom...";

struct audio_item
{
    QString name;
    QString path;
};

const std::map<QString, QString> AudioMap {
    {Audio_Default_1, ":/assets/sound/alarm-retro.wav"},
    {Audio_Default_2, ":/assets/sound/alarm-poehali.wav"},
};


// Used app name
const QString AppName = "QBreak";


class app_settings
{
public:
    struct selected_audio
    {
        QString name = Audio_Empty;
        QString path;
    };

    struct config
    {
        // Seconds
        int longbreak_interval              = Default_LongBreak_Interval;
        int longbreak_postpone_interval     = Default_LongBreak_PostponeInterval;
        int longbreak_length                = Default_LongBreak_Length;
        bool window_on_top                  = Default_WindowOnTop;
        bool verbose                        = Default_Verbose;
        bool autostart                      = Default_Autostart;
        QString preferred_monitor           = Default_Monitor;

        // This value can be path to audio file or empty or [embedded] string
        selected_audio play_audio;
        QString script_on_break_finish;
    };

    static void save(const config& cfg);
    static config load();
};


#endif // SETTINGS_H
