#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settings.h"

#include <QToolTip>
#include <QTimer>
#include <QWindow>

const QString ConversionError = "Integer value expected.";

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    init();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::init()
{
    setWindowTitle("Settings");

    auto c = app_settings::load();
    ui->mAutostartCheckbox->setChecked(c.autostart);
    ui->mWindowOnTopCheckbox->setChecked(c.window_on_top);
    ui->mBreakIntervalEdit->setText(QString::number(c.longbreak_interval / 60));
    ui->mBreakDurationEdit->setText(QString::number(c.longbreak_length / 60));
    ui->mPostponeTimeEdit->setText(QString::number(c.longbreak_postpone_interval / 60));
}

void SettingsDialog::accept()
{
    auto c = app_settings::load();
    c.autostart = ui->mAutostartCheckbox->isChecked();
    c.window_on_top = ui->mWindowOnTopCheckbox->isChecked();
    c.longbreak_interval = ui->mBreakIntervalEdit->text().toInt() * 60;
    c.longbreak_length = ui->mBreakDurationEdit->text().toInt() * 60;
    c.longbreak_postpone_interval = ui->mPostponeTimeEdit->text().toInt() * 60;
    app_settings::save(c);

    emit accepted();
}
