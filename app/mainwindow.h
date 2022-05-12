#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSystemTrayIcon>

#include <chrono>
#include "settings.h"
#include "settingsdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Start screen immediately with 15 seconds interval
    void test_1();

    // Run 60/60 seconds interval & duration
    void test_2();

private:
    Ui::MainWindow *ui;
    QTimer* mTimer;
    QTimer* mNotifyTimer;
    QTimer* mShowNotifyTimer;
    QTimer* mUpdateUITimer;
    QTimer* mProgressTimer;
    QSystemTrayIcon* mTrayIcon;
    SettingsDialog* mSettingsDialog;
    std::chrono::steady_clock::time_point mBreakStartTime;
    app_settings::config mAppConfig;
    int mPostponeCount;
    std::chrono::steady_clock::time_point mIdleStart;

    void init();
    void loadConfig();
    void applyConfig();
    void schedule();
    void createTrayIcon();
    void showMe();
    void hideMe();

    void startNotification();

    QIcon getAppIcon();
    QIcon getTrayIcon();

public slots:
    void onUpdateUI();
    void onLongBreakNotify();
    void onLongBreakStart();
    void onLongBreakPostpone();
    void onLongBreakEnd();

    void onProgress();
    void onNextBreak();
    void onSettings();
    void onAbout();
    void onExit();
};
#endif // MAINWINDOW_H
