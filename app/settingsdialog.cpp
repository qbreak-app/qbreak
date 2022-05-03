#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settings.h"

#include <QToolTip>
#include <QTimer>
#include <QWindow>
#include <QScreen>

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

    ui->mPreferredMonitorCombobox->addItem(Primary_Monitor, Primary_Monitor);
    int found_idx = 0;
    auto ql = QGuiApplication::screens();
    int screen_idx = 1;
    for (QScreen* s: ql)
    {
        ui->mPreferredMonitorCombobox->addItem(s->name() +" / " + s->model() + " " + s->manufacturer(), s->name());

        if (s->name() == c.preferred_monitor)
            found_idx = screen_idx;

        screen_idx++;
    }

    ui->mPreferredMonitorCombobox->setCurrentIndex(found_idx);

    ui->mAudioComboBox->addItem(Empty_Play_Audio);
    ui->mAudioComboBox->addItem(Embedded_Play_Audio);
    if (c.play_audio != Empty_Play_Audio && c.play_audio != Embedded_Play_Audio)
        ui->mAudioComboBox->addItem(c.play_audio);
    for (int i = 0; i < ui->mAudioComboBox->count(); i++)
        if (ui->mAudioComboBox->itemText(i) == c.play_audio)
            ui->mAudioComboBox->setCurrentIndex(i);

    ui->mScriptEdit->setText(c.script_on_break_finish);
}

void SettingsDialog::accept()
{
    auto c = app_settings::load();

    c.autostart = ui->mAutostartCheckbox->isChecked();
    c.window_on_top = ui->mWindowOnTopCheckbox->isChecked();
    c.longbreak_interval = ui->mBreakIntervalEdit->text().toInt() * 60;
    c.longbreak_length = ui->mBreakDurationEdit->text().toInt() * 60;
    c.longbreak_postpone_interval = ui->mPostponeTimeEdit->text().toInt() * 60;
    c.preferred_monitor = ui->mPreferredMonitorCombobox->currentData().toString();
    c.script_on_break_finish = ui->mScriptEdit->text();
    c.play_audio = ui->mAudioComboBox->currentText();

    app_settings::save(c);

    emit accepted();
}
