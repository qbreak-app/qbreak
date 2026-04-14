# QBreak

A lightweight break reminder app for Linux, built with C++17 and Qt (5/6).

## Project Structure

- `app/` — All application source code, UI files, assets, and the `.pro` project file
- `app/assets/` — Icons (`images/coffee_cup/`), sounds (`sound/`), desktop entry (`misc/`)
- `app/config.h` — Version constants and UI interval settings
- `scripts/` — Shell and Python build scripts, AppImage templates, release output

## Building

Requires Qt (5.15+ or 6.8+), qmake, X11 dev libraries (`libxss-dev`), and Wayland client libraries (`libwayland-dev`). The Wayland protocol stubs (`app/wayland/ext-idle-notify-v1-*`) are pre-generated and checked in, so `wayland-scanner` is not needed at build time.

Set `QT_HOME` to your Qt installation, then run the appropriate build script:

```sh
# Qt5 release
export QT_HOME=$HOME/tools/qt/5.15.2/gcc_64
cd scripts && ./build_linux.sh

# Qt6 release
export QT_HOME=$HOME/tools/qt/6.8.0/gcc_64
cd scripts && ./build_linux_qt6.sh

# Debug variants
./build_linux_debug.sh        # Qt5
./build_linux_qt6_debug.sh    # Qt6
```

Output: `scripts/releases/qbreak-<version>-x86_64.AppImage`

## Key Source Files

| File | Role |
|------|------|
| `app/main.cpp` | Entry point, logging setup |
| `app/mainwindow.cpp` | Core UI, break scheduling, state machine (None/Counting/Idle/Break) |
| `app/idle_tracking.cpp` | Idle detection: X11 XScreenSaver, Wayland `ext-idle-notify-v1` (KWin 5.26+, Mutter 45+, Sway, Hyprland), GNOME Mutter DBus fallback |
| `app/wayland/` | Vendored `ext-idle-notify-v1` protocol XML and pre-generated C client stubs |
| `app/settings.cpp` | QSettings-based config persistence |
| `app/settingsdialog.cpp` | Preferences dialog |
| `app/autostart.cpp` | XDG autostart and desktop integration |
| `app/runguard.cpp` | Single-instance protection via shared memory |
| `app/audio_support.cpp` | Break alarm audio playback (Qt5 only) |

## Architecture Notes

- App states: `None` → `Counting` → `Break` (or `Idle` if user is inactive)
- Idle detection: X11 via `dlopen` of libXss, Wayland via `ext-idle-notify-v1` with a `QSocketNotifier`-driven dispatch loop, GNOME via Mutter's DBus `IdleMonitor` (fallback for compositors lacking `ext-idle-notify-v1`)
- Qt6 audio support is not yet implemented
- Single instance enforced via `QSharedMemory`
- Config stored at `~/.config/voipobjects.com/QBreak.conf`
- Translations: English and Russian (`app/strings_en.ts`, `app/strings_ru.ts`)

## Testing

Built-in test functions in `mainwindow.h`:
- `test_1()` — Immediate break screen with 15-second interval
- `test_2()` — 60/60-second work/break cycle

No automated test suite exists.

## Code Style

- C++17 with Qt idioms (signals/slots, QObject hierarchy)
- qmake build system (`.pro` file), not CMake
- Platform-specific code guarded with `#ifdef` (`Q_OS_LINUX`, Qt version checks)
