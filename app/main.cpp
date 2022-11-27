#include "mainwindow.h"
#include "autostart.h"
#include "runguard.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    RunGuard guard("adjfaifaif");
    if ( !guard.tryToRun() )
        return 0;

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("voipobjects.com");
    QCoreApplication::setOrganizationDomain("voipobjects.com");
    QCoreApplication::setApplicationName("QBreak");

    QTranslator translator;
    bool ok = translator.load(":/i18n/strings_" + QLocale::system().name());

    if (ok)
      app.installTranslator(&translator);

    app.setQuitOnLastWindowClosed(false);


    // Put itself into app menu
    auto exe_path = QCoreApplication::applicationFilePath();//QFileInfo(QCoreApplication::arguments().front()).absoluteFilePath();
    const char* appimage = std::getenv("APPIMAGE");
    if (appimage != nullptr)
        exe_path = appimage;

    appmenu::install(exe_path.toStdString());

    // Main window is full screen window, so start with tray icon only
    MainWindow w;
    w.hide();

    int retcode = app.exec();

    return retcode;
}
