#include "mainwindow.h"
#include "autostart.h"
#include "runguard.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <QFileInfo>
#include <QDateTime>

#if defined(TARGET_LINUX)
# include <syslog.h>
#endif

// Handler for Qt log messages that sends output to syslog as well as standard error.
void SyslogMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    std::string timePrefix = QDateTime::currentDateTime().toString().toStdString();

    QByteArray localMsg = msg.toLocal8Bit();
      switch (type) {
      case QtDebugMsg:
          fprintf(stderr, "debug: %s %s\n", timePrefix.c_str(), localMsg.constData());
#if defined(TARGET_LINUX)
          syslog(LOG_DEBUG, "debug: %s", localMsg.constData());
#endif
          break;
      case QtInfoMsg:
          fprintf(stderr, "info: %s %s\n", timePrefix.c_str(), localMsg.constData());
#if defined(TARGET_LINUX)
          syslog(LOG_INFO, "info: %s", localMsg.constData());
#endif
          break;
      case QtWarningMsg:
          fprintf(stderr, "warning: %s %s\n", timePrefix.c_str(), localMsg.constData());
#if defined(TARGET_LINUX)
          syslog(LOG_WARNING, "warning: %s", localMsg.constData());
#endif
          break;
      case QtCriticalMsg:
          fprintf(stderr, "critical: %s %s\n", timePrefix.c_str(), localMsg.constData());
#if defined(TARGET_LINUX)
          syslog(LOG_CRIT, "critical: %s", localMsg.constData());
#endif
          break;
      case QtFatalMsg:
          fprintf(stderr, "fatal: %s %s\n", timePrefix.c_str(), localMsg.constData());
#if defined(TARGET_LINUX)
          syslog(LOG_ALERT, "fatal: %s", localMsg.constData());
#endif
          abort();
      }
}


int main(int argc, char *argv[])
{
    RunGuard guard("QBreak app - runguard");
    if ( !guard.tryToRun() )
        return 0;

    // Needed to enable logging to syslog or journald.
    qInstallMessageHandler(SyslogMessageHandler);

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
    auto exe_path = QCoreApplication::applicationFilePath();
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
