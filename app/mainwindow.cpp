#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "settings.h"
#include "autostart.h"
#include "aboutdlg.h"
#include "config.h"

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

static bool isDarkTheme()
{
    // Thanks to stackoverflow ! Another idea is to use https://successfulsoftware.net/2021/03/31/how-to-add-a-dark-theme-to-your-qt-application/
    auto label = QLabel("am I in the dark?");
    auto text_hsv_value = label.palette().color(QPalette::WindowText).value();
    auto bg_hsv_value = label.palette().color(QPalette::Background).value();
    bool dark_theme_found = text_hsv_value > bg_hsv_value;

    return dark_theme_found;
}

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

    // ToDo: finish the multiple monitor support
    /*QDesktopWidget *widget = QApplication::desktop();
    qDebug() << "Number of screens:" << widget->screenCount();

    connect(widget, &QDesktopWidget::screenCountChanged, this, [](int screenCount){});
    if (widget->screenCount() > 1) {
    } else if (widget->screenCount() == 1){
    }*/

    // Tray icon
    createTrayIcon();

    // Latest config
    loadConfig();

    // Settings dialog
    mSettingsDialog = nullptr;

    // No postpone attempts yet
    mPostponeCount = 0;

    // Timer
    mTimer = new QTimer(this);
    mTimer->setTimerType(Qt::TimerType::CoarseTimer);
    mTimer->setSingleShot(true);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(onLongBreakStart()));

    mNotifyTimer = new QTimer(this);
    mNotifyTimer->setTimerType(Qt::TimerType::CoarseTimer);
    mNotifyTimer->setSingleShot(true);
    connect(mNotifyTimer, SIGNAL(timeout()), this, SLOT(onLongBreakNotify()));


    mUpdateUITimer = new QTimer(this);
    mUpdateUITimer->setTimerType(Qt::TimerType::CoarseTimer);
    mUpdateUITimer->setSingleShot(false);
    mUpdateUITimer->setInterval(std::chrono::seconds(INTERVAL_UPDATE_UI));
    connect(mUpdateUITimer, SIGNAL(timeout()), this, SLOT(onUpdateUI()));
    mUpdateUITimer->start();

    mProgressTimer = new QTimer(this);
    mProgressTimer->setInterval(std::chrono::milliseconds(INTERVAL_UPDATE_PROGRESS));
    mProgressTimer->setSingleShot(false);
    connect(mProgressTimer, SIGNAL(timeout()), this, SLOT(onProgress()));

    connect(ui->mPostponeButton, SIGNAL(clicked()), this, SLOT(onLongBreakPostpone()));
    connect(ui->mSkipButton, SIGNAL(clicked()), this, SLOT(onLongBreakEnd()));

    // Use the latest config
    applyConfig();

    // Refresh UI
    onUpdateUI();
}

void MainWindow::loadConfig()
{
    app_settings settings;
    mAppConfig = settings.load();
}

void MainWindow::applyConfig()
{
    if (mTimer)
    {
        if (mTimer->interval() != mAppConfig.longbreak_interval)
        {
            mTimer->stop();
            mTimer->setInterval(std::chrono::seconds(mAppConfig.longbreak_interval));
            mTimer->start();

            mNotifyTimer->stop();
            mNotifyTimer->setInterval(std::chrono::seconds(mAppConfig.longbreak_interval - 30));
            mNotifyTimer->start();
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

void MainWindow::schedule()
{
    ;
}

void MainWindow::test_1()
{
    // 10 seconds test break
    mAppConfig.longbreak_length = 10;
    mAppConfig.longbreak_postpone_interval = 10;
    mAppConfig.longbreak_interval = 10;

    mAppConfig.window_on_top = true;
    mAppConfig.verbose = true;
    applyConfig();
    onUpdateUI();
    onLongBreakStart();
}

void MainWindow::test_2()
{
    // 60 seconds test break
    mAppConfig.longbreak_length = 60;
    mAppConfig.longbreak_postpone_interval = 60;
    mAppConfig.longbreak_interval = 240;

    mAppConfig.window_on_top = true;
    mAppConfig.verbose = true;

    applyConfig();
    onUpdateUI();
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
            qDebug() << "Window moved to screen " << screen->name() + " / " + screen->model() + " " + screen->manufacturer();
        }
    }
    showFullScreen();
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
        return QObject::tr("%1 minutes").arg(seconds / 60);
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
}

void MainWindow::onUpdateUI()
{
    if (mTrayIcon)
    {
        if (mProgressTimer->isActive())
        {
            // Break is in effect now
            mTrayIcon->setToolTip(QString());
        }
        else
        if (mTimer->isActive())
        {
            auto remaining_milliseconds = mTimer->remainingTime();
            if (remaining_milliseconds == 0)
                mTrayIcon->setToolTip(tr("Less than a minute left until the next break."));
            else
                mTrayIcon->setToolTip(tr("There are %1 minutes left until the next break.").arg(remaining_milliseconds / 1000 / 60));
        }
    }

    ui->mSkipButton->setVisible(mPostponeCount > 0);
}

void MainWindow::onLongBreakNotify()
{
    mTrayIcon->showMessage(tr("New break"),
                           tr("New break will start in %1 secs").arg(Default_Notify_Length),
                           getAppIcon(),
                           INTERVAL_NOTIFICATION);
}

void MainWindow::onLongBreakStart()
{
    qDebug() << "Long break starts for " << secondsToText(mAppConfig.longbreak_postpone_interval);

    ui->mPostponeButton->setText(tr("Postpone for ") + secondsToText(mAppConfig.longbreak_postpone_interval));
    showMe();

    mBreakStartTime = std::chrono::steady_clock::now();

    // Start progress bar
    mProgressTimer->start();

    // Refresh UI
    onUpdateUI();
}

void MainWindow::onLongBreakEnd()
{
    qDebug() << "Long break ends.";

    // Reset postpone counter
    mPostponeCount = 0;

    // Prepare to next triggering
    ui->mProgressBar->setValue(0);

    // Hide the window
    hideMe();

    // Start new timer
    mTimer->start(std::chrono::seconds(mAppConfig.longbreak_interval));
    mNotifyTimer->start(std::chrono::seconds(mAppConfig.longbreak_interval - 30));

    // Refresh UI
    onUpdateUI();
}

void MainWindow::onLongBreakPostpone()
{
    qDebug() << "Long break postponed.";
    mPostponeCount++;

    hideMe();

    mProgressTimer->stop();
    ui->mProgressBar->setValue(0);

    // Start timer again
    mTimer->start(std::chrono::seconds(mAppConfig.longbreak_postpone_interval));
    mNotifyTimer->start(std::chrono::seconds(mAppConfig.longbreak_postpone_interval - 30));

    // Refresh UI
    onUpdateUI();
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

    ui->mRemainingLabel->setText(QString("Remaining: ") + secondsToText(remaining));

    if (percents > 100)
    {
        mProgressTimer->stop();
        onLongBreakEnd();
    }
    else
        showMe();
}

void MainWindow::onNextBreak()
{
    mTimer->stop();
    mNotifyTimer->stop();

    onLongBreakStart();
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
