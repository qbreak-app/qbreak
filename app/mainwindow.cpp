#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "settings.h"
#include "autostart.h"
#include "aboutdlg.h"
#include "config.h"
#include "audio_support.h"
#include "idle_tracking.h"

#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QDebug>
#include <QDesktopWidget>
#include <QSvgGenerator>
#include <QPalette>
#include <QScreen>
#include <QWindow>
#include <QFileInfo>
#include <QSound>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDateTime>

#include <string>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init();
}

MainWindow::~MainWindow()
{
    if (mSettingsDialog)
        delete mSettingsDialog;
    delete ui;
}

/*static bool isDarkTheme()
{
    // Thanks to stackoverflow ! Another idea is to use https://successfulsoftware.net/2021/03/31/how-to-add-a-dark-theme-to-your-qt-application/
    auto label = QLabel("am I in the dark?");
    auto text_hsv_value = label.palette().color(QPalette::WindowText).value();
    auto bg_hsv_value = label.palette().color(QPalette::Background).value();
    bool dark_theme_found = text_hsv_value > bg_hsv_value;

    return dark_theme_found;
}*/

QIcon MainWindow::getAppIcon()
{
    QIcon app_icon(QPixmap(QString(":/assets/images/coffee_cup/icon_128x128.png")));

    return app_icon;
}

QIcon MainWindow::getTrayIcon()
{
    // QString app_icon_name = isDarkTheme() ? "app_icon_dark.png" : "app_icon_light.png";
    // QIcon app_icon(QPixmap(QString(":/assets/images/%1").arg(app_icon_name)));
    // return app_icon;

    return QIcon(QPixmap(":/assets/images/coffee_cup/icon_64x64.png"));
}

void MainWindow::init()
{
    // Icon
    setWindowIcon(getAppIcon());

    // Tray icon
    createTrayIcon();

    // Latest config
    loadConfig();

    // Settings dialog
    mSettingsDialog = nullptr;

    // No postpone attempts yet
    mPostponeCount = 0;

    // Idle is not detected yet
    mLastIdleMilliseconds = 0;

    // Timer to start break
    mBreakStartTimer = new QTimer(this);
    mBreakStartTimer->setTimerType(Qt::TimerType::CoarseTimer);
    mBreakStartTimer->setSingleShot(true);
    connect(mBreakStartTimer, &QTimer::timeout, this, [this](){shiftTo(AppState::Break);});

    // Timer to run notification about upcoming break
    mBreakNotifyTimer = new QTimer(this);
    mBreakNotifyTimer->setTimerType(Qt::TimerType::CoarseTimer);
    mBreakNotifyTimer->setSingleShot(true);
    connect(mBreakNotifyTimer, SIGNAL(timeout()), this, SLOT(onLongBreakNotify()));

    // Just update UI once per minute
    mUpdateUITimer = new QTimer(this);
    mUpdateUITimer->setTimerType(Qt::TimerType::CoarseTimer);
    mUpdateUITimer->setSingleShot(false);
    mUpdateUITimer->setInterval(std::chrono::seconds(INTERVAL_UPDATE_UI));
    connect(mUpdateUITimer, SIGNAL(timeout()), this, SLOT(onUpdateUI()));
    mUpdateUITimer->start();

    // Timer to draw progress bar during the break
    mProgressTimer = new QTimer(this);
    mProgressTimer->setInterval(std::chrono::milliseconds(INTERVAL_UPDATE_PROGRESS));
    mProgressTimer->setSingleShot(false);
    connect(mProgressTimer, SIGNAL(timeout()), this, SLOT(onProgress()));

    connect(ui->mPostponeButton, SIGNAL(clicked()), this, SLOT(onLongBreakPostpone()));
    connect(ui->mSkipButton, &QPushButton::clicked, this, [this](){shiftTo(AppState::Counting);});

    // Use the latest config
    applyConfig();

    shiftTo(AppState::Counting);
}

static int str_to_seconds(const QString& s)
{
    if (s.isEmpty())
        throw std::runtime_error("Bad parameter value.");

    if (s.back() == 'h')
        return s.leftRef(s.size()-1).toInt() * 3600;
    if (s.back() == 'm')
        return s.leftRef(s.size()-1).toInt() * 60;
    if (s.back() == 's')
        return s.leftRef(s.size()-1).toInt();

    if (s.back().isDigit())
        return s.toInt();

    throw std::runtime_error("Bad parameter value");
}

void MainWindow::loadConfig()
{
    mAppConfig = app_settings::load();

    // Check for command line options
    QCommandLineParser parser;
    QCommandLineOption
            param_break("break-duration", "Long break duration.", "break-duration"),
            param_work("work-duration", "Work time duration.", "work-duration"),
            param_idle("idle-timeout", "Idle timeout.", "idle-timeout");
    parser.addOptions({param_break, param_work, param_idle});

    parser.process(*QApplication::instance());
    if (parser.isSet(param_break))
        mAppConfig.longbreak_length = str_to_seconds(parser.value(param_break));
    if (parser.isSet(param_work))
        mAppConfig.longbreak_interval = str_to_seconds(parser.value(param_work));
    if (parser.isSet(param_idle))
        mAppConfig.idle_timeout = str_to_seconds(parser.value(param_idle));
}

void MainWindow::applyConfig()
{
    if (mBreakStartTimer)
    {
        if (mBreakStartTimer->interval() != mAppConfig.longbreak_interval)
        {
            mBreakStartTimer->stop();
            mBreakStartTimer->setInterval(std::chrono::seconds(mAppConfig.longbreak_interval));
            mBreakStartTimer->start();

            mBreakNotifyTimer->stop();
            mBreakNotifyTimer->setInterval(std::chrono::seconds(mAppConfig.longbreak_interval - 30));
            mBreakNotifyTimer->start();
        }
    }

    if (mAppConfig.window_on_top)
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    else
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

    if (mAppConfig.autostart)
    {
        auto path_to_me = QFileInfo(QApplication::arguments().front()).absoluteFilePath();
        autostart::enable(path_to_me.toStdString());
    }
    else
        autostart::disable();
}

void MainWindow::showMe()
{
    QScreen* screen = nullptr;
    if (mAppConfig.preferred_monitor == Primary_Monitor)
        screen = QGuiApplication::primaryScreen();
    else
    {
        auto screen_list = QGuiApplication::screens();
        auto screen_iter = std::find_if(screen_list.begin(), screen_list.end(), [this](QScreen* s){return s->name() == mAppConfig.preferred_monitor;});
        if (screen_iter != screen_list.end())
            screen = *screen_iter;
    }

    if (screen)
    {
        show();
        if (windowHandle())
        {
            windowHandle()->setScreen(screen);
            setGeometry(screen->geometry());
            // qDebug() << "Window moved to screen " << screen->name() + " / " + screen->model() + " " + screen->manufacturer();
        }
    }
    // else
    //    qDebug() << "Screen not found!";

#if defined(DEBUG)
    showFullScreen();
    //showMaximized();
#else
    showFullScreen();
#endif
}

void MainWindow::hideMe()
{
    this->hide();
}

static QString secondsToText(int seconds)
{
    if (seconds < 60)
        return QObject::tr("%1 seconds").arg(seconds);
    else
    {
        int minutes = int(float(seconds) / 60 + 0.5f);
        return QObject::tr("%1 minutes").arg(minutes);
    }
}

void MainWindow::createTrayIcon()
{
    mTrayIcon = new QSystemTrayIcon(this);
    QMenu* menu = new QMenu(this);

    QAction* nextBreakAction = new QAction(tr("Start next break"), menu);
    connect(nextBreakAction, SIGNAL(triggered()), this, SLOT(onNextBreak()));

    QAction* settingsAction = new QAction(tr("Settings"), menu);
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(onSettings()));

    QAction* aboutAction = new QAction(tr("About"), menu);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(onAbout()));

    QAction* exitAction = new QAction(tr("Exit"), menu);
    connect(exitAction, SIGNAL(triggered()), this, SLOT(onExit()));

    menu->addAction(nextBreakAction);
    menu->addAction(settingsAction);
    menu->addAction(aboutAction);
    menu->addAction(exitAction);

    mTrayIcon->setContextMenu(menu);
    mTrayIcon->setIcon(getTrayIcon());
    mTrayIcon->setToolTip(AppName);
    mTrayIcon->show();
    connect(mTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(onTrayIconActivated(QSystemTrayIcon::ActivationReason)));
}

static int msec2min(int msec)
{
    float min_f = float(msec) / 1000 / 60;
    return (int)(min_f + 0.5f);
}

QString state2str(AppState state)
{
    switch (state)
    {
    case AppState::None:
        return "None";
    case AppState::Break:
        return "Break";
    case AppState::Idle:
        return "Idle";
    case AppState::Counting:
        return "Counting";
    }
    return QString();
}

static void dispatchToMainThread(std::function<void()> callback)
{
    // any thread
    QTimer* timer = new QTimer();
    timer->moveToThread(qApp->thread());
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=]()
    {
        // main thread
        callback();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

void MainWindow::shiftTo(AppState newState)
{
    if (newState == mState)
        return;
    qDebug() << state2str(mState) << " -> " << state2str(newState);

    switch (newState)
    {
    case AppState::None:
        // Do nothing, app is not started
        break;

    case AppState::Idle:
        onIdleStart();
        break;

    case AppState::Break:
        // Break is active
        onLongBreakStart();
        break;

    case AppState::Counting:
        // Working, break is closing
        if (mState == AppState::Break)
            onLongBreakEnd();
        else
        if (mState == AppState::Idle)
            onIdleEnd();
        break;
    }

    mState = newState;

    // Run deferred in main UI thread
    dispatchToMainThread([this](){
        onUpdateUI();
    });

}

void MainWindow::onUpdateUI()
{
    qDebug() << "UI timer fired.";

    int idle_milliseconds = 0;
    switch (mState) {
    case AppState::None:
        // Do nothing, app is not started
        break;

    case AppState::Idle:
        // Detected idle, don't count this time as working
        // But check - maybe idle is over
        idle_milliseconds = get_idle_time_dynamically();
        qDebug() << "Idle found: " << idle_milliseconds / 1000 << "s";

        if (idle_milliseconds < mAppConfig.idle_timeout * 1000)
        {
            shiftTo(AppState::Counting);
            return;
        }
        break;

    case AppState::Break:
        // Break is active
        if (mTrayIcon)
            mTrayIcon->setToolTip(QString());
        break;

    case AppState::Counting:
        // Working, break is closing
        // Check maybe it is idle ?
        if (!mIdleStart && mAppConfig.idle_timeout)
        {
            idle_milliseconds = get_idle_time_dynamically();
            qDebug() << "Idle found: " << idle_milliseconds / 1000 << "s";

            if (idle_milliseconds >= mAppConfig.idle_timeout * 1000)
            {
                shiftTo(AppState::Idle);
                return;
            }
        }

        // Update tray icon
        if (mTrayIcon) {
            auto remaining_milliseconds = mBreakStartTimer->remainingTime();
            if (remaining_milliseconds < 60000)
                mTrayIcon->setToolTip(
                            tr("Less than a minute left until the next break."));
            else
                mTrayIcon->setToolTip(
                            tr("There are %1 minutes left until the next break.")
                            .arg(msec2min(remaining_milliseconds)));
        }
        else
            qDebug() << "No tray icon available.";

        break;
    }

    ui->mSkipButton->setVisible(mPostponeCount > 0);
}

void MainWindow::onLongBreakNotify()
{
    mTrayIcon->showMessage(tr("New break"),
                           tr("New break will start in %1 secs").arg(Default_Notify_Length),
                           getAppIcon(),
                           INTERVAL_NOTIFICATION * 1000);
}

void MainWindow::onLongBreakStart()
{
    mBreakStartTimer->stop();
    mBreakNotifyTimer->stop();
    qDebug() << "Stop main and notify timers.";

    // Reset idle counter
    mIdleStart.reset();

    // Show the button "Postpone"
    ui->mPostponeButton->setText(tr("Postpone for ") + secondsToText(mAppConfig.longbreak_postpone_interval));

    // Show the screen
    showMe();

    // Save start time
    mBreakStartTime = std::chrono::steady_clock::now();

    // Start progress bar
    mProgressTimer->start();

    // Update title immediate
    onProgress();
}

void MainWindow::onLongBreakEnd()
{
    // qDebug() << "Long break ends.";

    // Prepare to next triggering
    ui->mProgressBar->setValue(0);

    // Hide the window
    hideMe();

    mProgressTimer->stop();

    // Start new timer
    if (!mBreakStartTimer->isActive())
    {
        mBreakStartTimer->start(std::chrono::seconds(mAppConfig.longbreak_interval));
        qDebug() << "Start main timer for " << mAppConfig.longbreak_interval << " seconds.";
    }
    if (!mBreakNotifyTimer->isActive())
    {
        mBreakNotifyTimer->start(std::chrono::seconds(mAppConfig.longbreak_interval - 30));
        qDebug() << "Start notify timer for " << mAppConfig.longbreak_interval - 30 << " seconds.";
    }

    // Play selected audio. When break is postponed - audio is not played
    if (0 == mPostponeCount)
        play_audio(mAppConfig.play_audio);

    // Run script
    if (!mAppConfig.script_on_break_finish.isEmpty())
    {
        int retcode = system(mAppConfig.script_on_break_finish.toStdString().c_str());
        if (retcode != 0)
            qDebug() << "User script exited with error code " << retcode;
    }
}

void MainWindow::onLongBreakPostpone()
{
    // qDebug() << "Long break postponed.";
    mPostponeCount++;

    hideMe();

    mProgressTimer->stop();
    ui->mProgressBar->setValue(0);

    // Start timer again
    mBreakStartTimer->stop();
    mBreakStartTimer->start(std::chrono::seconds(mAppConfig.longbreak_postpone_interval));
    mBreakNotifyTimer->stop();
    mBreakNotifyTimer->start(std::chrono::seconds(mAppConfig.longbreak_postpone_interval - 30));

    shiftTo(AppState::Counting);
}

void MainWindow::onProgress()
{
    auto timestamp = std::chrono::steady_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::seconds>(timestamp - mBreakStartTime).count();

    int percents = (delta / float(mAppConfig.longbreak_length) * 100);
    ui->mProgressBar->setValue(percents);
    int remaining = mAppConfig.longbreak_length - delta;
    if (remaining < 0)
        remaining = 0;

    auto text = QString("Remaining: ") + secondsToText(remaining);
    ui->mRemainingLabel->setText(text);
    if (mTrayIcon)
        mTrayIcon->setToolTip(text);

    if (percents > 100)
    {
        mProgressTimer->stop();
        mPostponeCount = 0; // Reset postpone counter
        shiftTo(AppState::Counting);
    }
    else
        showMe();
}

void MainWindow::onNextBreak()
{
    shiftTo(AppState::Break);
}

void MainWindow::onIdleStart()
{
    if (mState != AppState::Counting)
        return;

    // Detected idle
    // Timestamp when idle started
    mIdleStart = std::chrono::steady_clock::now();

    // How much working time remains
    mRemainingWorkInterval = mBreakStartTimer->remainingTime() / 1000;

    // Stop main & notify timers
    mBreakStartTimer->stop();
    mBreakNotifyTimer->stop();
    qDebug() << "Stop main and notify timers. Remaining time is " << mRemainingWorkInterval.value() << "s";
}

void MainWindow::onIdleEnd()
{
    if (mState != AppState::Idle)
        return;

    mIdleStart.reset();

    // Update timer(s) duration
    if (mRemainingWorkInterval)
    {
        qDebug() << "Reset main timer to " << *mRemainingWorkInterval << "s";
        mBreakStartTimer->setInterval(std::chrono::seconds(*mRemainingWorkInterval));

        if (mRemainingWorkInterval > INTERVAL_NOTIFICATION)
        {
            mBreakNotifyTimer->setInterval(std::chrono::seconds(*mRemainingWorkInterval - INTERVAL_NOTIFICATION));
            qDebug() << "Reset notify timer to " << *mRemainingWorkInterval - INTERVAL_NOTIFICATION << "s";
        }
        mRemainingWorkInterval.reset();
    }
    else
    {
        mBreakStartTimer->setInterval(std::chrono::seconds(mAppConfig.longbreak_interval));
        if (mRemainingWorkInterval > INTERVAL_NOTIFICATION)
            mBreakNotifyTimer->setInterval(std::chrono::seconds(mAppConfig.longbreak_interval - INTERVAL_NOTIFICATION));
    }

    if (!mBreakStartTimer->isActive())
        mBreakStartTimer->start();
    if (!mBreakNotifyTimer->isActive())
        mBreakNotifyTimer->start();
}

void MainWindow::onSettings()
{
    if (mSettingsDialog == nullptr)
    {
        // Settings dialog must be top level window to be positioned on the primary screen
        mSettingsDialog = new SettingsDialog(nullptr);
        connect(mSettingsDialog, &QDialog::accepted, [this]()
        {
            mAppConfig = app_settings::load();
            applyConfig();
            mSettingsDialog->hide();
            onUpdateUI();
        });

        connect(mSettingsDialog, &QDialog::rejected, [this]()
        {
            mSettingsDialog->hide();
        });
    }
    // Move to primary screen
    mSettingsDialog->show();
    QScreen* screen = QGuiApplication::primaryScreen();
    mSettingsDialog->windowHandle()->setScreen(screen);
}

void MainWindow::onAbout()
{
    AboutDlg dlg;
    dlg.exec();
}

void MainWindow::onExit()
{
    this->close();
    QApplication::exit();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    // Show context menu on single click
    case QSystemTrayIcon::Trigger:
        mTrayIcon->contextMenu()->popup(QCursor::pos());
        break;

    case QSystemTrayIcon::Unknown:
    case QSystemTrayIcon::Context:
    case QSystemTrayIcon::DoubleClick:
    case QSystemTrayIcon::MiddleClick:
        break;
    }
}
