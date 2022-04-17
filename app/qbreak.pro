QT       += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 lrelease embed_translations

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdlg.cpp \
    autostart.cpp \
    main.cpp \
    mainwindow.cpp \
    settings.cpp \
    settingsdialog.cpp \
    runguard.cpp

HEADERS += \
    aboutdlg.h \
    autostart.h \
    config.h \
    mainwindow.h \
    settings.h \
    settingsdialog.h \
    runguard.h

FORMS += \
    aboutdlg.ui \
    mainwindow.ui \
    settingsdialog.ui

RESOURCES = qbreak.qrc

TRANSLATIONS = strings.ts strings_en.ts strings_ru.ts

unix:!macx: DEFINES += TARGET_LINUX

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
