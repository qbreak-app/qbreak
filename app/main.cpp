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

    QCoreApplication::setOrganizationName("qbreak.com");
    QCoreApplication::setOrganizationDomain("qbreak.com");
    QCoreApplication::setApplicationName("QBreak");

    QTranslator translator;
    bool ok = translator.load(":/i18n/strings_" + QLocale::system().name());

    if (ok)
      app.installTranslator(&translator);

    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    QCommandLineOption test_1(QStringList() << "t1" << "test_1");
    parser.addOption(test_1);

    // Run app with break intervals & duration 60 seconds.
    // This should trigger the notification in 30 seconds before breaks.
    QCommandLineOption test_2(QStringList() << "t2" << "test_2");
    parser.addOption(test_2);

    parser.process(app);

    // Put itself into app menu
    auto exe_path = QFileInfo(QCoreApplication::arguments().front()).absoluteFilePath();
    const char* appimage = std::getenv("APPIMAGE");
    if (appimage != nullptr)
        exe_path = appimage;

    appmenu::install(exe_path.toStdString());

    // Main window is full screen window, so start with tray icon only
    MainWindow w;
    w.hide();

    if (parser.isSet(test_1))
        w.test_1();
    else
    if (parser.isSet(test_2))
        w.test_2();

    int retcode = app.exec();

    return retcode;
}
