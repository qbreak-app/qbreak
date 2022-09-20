#include "aboutdlg.h"
#include "ui_aboutdlg.h"
#include "config.h"

AboutDlg::AboutDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDlg)
{
    ui->setupUi(this);

    ui->mAppIcon->setPixmap(QPixmap(":/assets/images/coffee_cup/icon_96x96"));
    auto version_text = QString("Version %1.%2.%3")
            .arg(QBREAK_VERSION_MAJOR)
            .arg(QBREAK_VERSION_MINOR)
            .arg(QBREAK_VERSION_SUFFIX);
#if defined(DEBUG)
    version_text += ". Debug build.";
#endif
    ui->mAppVersionLabel->setText(version_text);
}

AboutDlg::~AboutDlg()
{
    delete ui;
}
