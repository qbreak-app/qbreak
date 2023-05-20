#include "audio_support.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QSound>

extern void play_audio(const app_settings::selected_audio& item)
{
    // Play audio

    if (item.name != Audio_Empty && item.name != Audio_Custom)
    {
        // Find bundled audio
        auto iter = AudioMap.find(item.name);
        if (iter != AudioMap.end())
            QSound::play(iter->second);
    }
    else
    if (item.name == Audio_Custom && !item.path.isEmpty())
        QSound::play(item.path);
}
#else
extern void play_audio(const app_settings::selected_audio& item)
{}
#endif
