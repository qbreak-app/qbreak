QT       += core gui svg multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets dbus

CONFIG += c++17 lrelease embed_translations

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdlg.cpp        \
    autostart.cpp       \
    main.cpp            \
    mainwindow.cpp      \
    settings.cpp        \
    settingsdialog.cpp  \
    runguard.cpp        \
    audio_support.cpp   \
    idle_tracking.cpp


HEADERS += \
    aboutdlg.h          \
    autostart.h         \
    config.h            \
    mainwindow.h        \
    settings.h          \
    settingsdialog.h    \
    runguard.h          \
    audio_support.h     \
    idle_tracking.h


FORMS += \
    aboutdlg.ui         \
    mainwindow.ui       \
    settingsdialog.ui

RESOURCES = qbreak.qrc

TRANSLATIONS = strings.ts strings_en.ts strings_ru.ts

unix:!macx: DEFINES += TARGET_LINUX # USE_WAYLAND

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# X11 and wayland libraries
unix:LIBS += -L/usr/X11R6/lib/          \
             -lX11 -lXext -lXss -ldl

# When using wayland:
# unix:LIBS +=  -L/usr/local/lib           \
#               -lwayland-client-unstable++ -lwayland-client-extra++ -lwayland-client++


