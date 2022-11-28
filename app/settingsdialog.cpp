#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settings.h"
#include "audio_support.h"

#include <QToolTip>
#include <QTimer>
#include <QWindow>
#include <QScreen>
#include <QFileDialog>
#include <QStandardPaths>

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
    mSkipAudioChangeEvent = true;

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

    // Fill audio combo box
    auto audios = {Audio_Empty, Audio_Retro, Audio_Gagarin, Audio_Custom};
    ui->mAudioComboBox->addItems(audios);
    mCustomAudioIdx = audios.size() - 1;

    // Preselect active audio
    for (int i = 0; i < ui->mAudioComboBox->count(); i++)
        if (ui->mAudioComboBox->itemText(i) == c.play_audio.name)
            ui->mAudioComboBox->setCurrentIndex(i);

    mCustomAudioPath = c.play_audio.path;

    ui->mScriptEdit->setText(c.script_on_break_finish);
    connect(ui->mAudioComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onAudioIndexChanged(int)));

    // Idle timeout
    ui->mIdleTimeoutSpinbox->setValue(c.idle_timeout);

    mSkipAudioChangeEvent = false;
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
    c.play_audio.name = ui->mAudioComboBox->currentText();
    if (c.play_audio.name == Audio_Custom)
        c.play_audio.path = mCustomAudioPath;

    c.idle_timeout = ui->mIdleTimeoutSpinbox->value();
    app_settings::save(c);

    emit accepted();
}

void SettingsDialog::onAudioIndexChanged(int idx)
{
    if (mSkipAudioChangeEvent)
        return;

    if (idx == mCustomAudioIdx)
    {
        // Ask about path to audio file
        auto home = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        auto path = QFileDialog::getOpenFileName(this, tr("Select audio file"), home, tr("Sound files(*.wav *.mp3 *.ogg)"));
        if (!path.isEmpty())
        {
            mCustomAudioPath = path;
            // ToDo: show message "audio is selected"
        }
        else
        {
            // ToDo: show message "audio is not selected"
        }
    }

    // Play selected audio to ensure this is fine
    play_audio({.name = ui->mAudioComboBox->itemText(idx), .path = mCustomAudioPath});

}
