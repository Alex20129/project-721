#include "groupsettingsdialog.h"
#include "ui_groupsettingsdialog.h"
#include <QDebug>

GroupSettingsDialog::GroupSettingsDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::GroupSettingsDialog)
{
	pGroupID=-1;
    ui->setupUi(this);
}

GroupSettingsDialog::~GroupSettingsDialog()
{
    delete ui;
}

void GroupSettingsDialog::showGroupSettings(int group_id)
{
	qInfo()<<"pGroupID "<<pGroupID;
	pGroupID=group_id;
	qInfo()<<"pGroupID "<<pGroupID;
}

void GroupSettingsDialog::on_buttonBox_accepted()
{
    quint16 apiPor=static_cast<quint16>(ui->apiport->text().toUInt());
    quint16 webPor=static_cast<quint16>(ui->webport->text().toUInt());
    emit(groupDataObtained(ui->title->text(), ui->description->toPlainText(), ui->username->text(), ui->password->text(), apiPor, webPor));
}
