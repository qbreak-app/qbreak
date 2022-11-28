#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSystemTrayIcon>

#include <chrono>
#include <optional>

#include "settings.h"
#include "settingsdialog.h"

// Possible app states
enum class AppState
{
    None,
    Counting,
    Idle,
    Break
};

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
    QTimer* mBreakStartTimer;           // Main timer - triggers when break occurs
    QTimer* mBreakNotifyTimer;          // Timer to show notification from system tray
    QTimer* mUpdateUITimer;             // Update UI timer - triggers every minute to update UI and checks for idle
    QTimer* mProgressTimer;             // Break progress timer - updates an UI
    QSystemTrayIcon* mTrayIcon;
    SettingsDialog* mSettingsDialog;

    std::chrono::steady_clock::time_point mBreakStartTime;

    // How much milliseconds remains for main break
    std::optional<int> mRemainingWorkInterval;

    app_settings::config mAppConfig;
    int mPostponeCount = 0;

    // Time when idle was started
    std::optional<std::chrono::steady_clock::time_point> mIdleStart;

    int mLastIdleMilliseconds = 0;

    AppState mState = AppState::None;

    void init();
    void loadConfig();
    void applyConfig();
    void createTrayIcon();
    void showMe();
    void hideMe();

    void startNotification();

    QIcon getAppIcon();
    QIcon getTrayIcon();

    // Function to switch state
    void shiftTo(AppState state);

public slots:
    void onUpdateUI();
    void onLongBreakNotify();
    void onLongBreakStart();
    void onLongBreakPostpone();
    void onLongBreakEnd();
    void onIdleStart();
    void onIdleEnd();

    void onProgress();
    void onNextBreak();
    void onSettings();
    void onAbout();
    void onExit();

    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
};
#endif // MAINWINDOW_H
