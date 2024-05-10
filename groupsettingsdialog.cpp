#include "groupsettingsdialog.h"
#include "ui_groupsettingsdialog.h"
#include "logger.hpp"
#include "mainwindow.hpp"

GroupSettingsDialog::GroupSettingsDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::GroupSettingsDialog)
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	pGroupID=-1;
    ui->setupUi(this);
}

GroupSettingsDialog::~GroupSettingsDialog()
{
    delete ui;
}

void GroupSettingsDialog::showGroupSettings(int group_id)
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	pGroupID=group_id;
	ASICTableWidget *group=gMainWin->GroupTabsWidgets->at(group_id);
	ui->title->setText(group->Title);
	ui->description->setText(group->Description);
	ui->username->setText(group->UserName);
	ui->password->setText(group->Password);
	ui->apiport->setText(QString::number(group->APIPort));
	ui->webport->setText(QString::number(group->WebPort));
	this->show();
}

void GroupSettingsDialog::on_buttonBox_accepted()
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *newGroupWidget;
	if(pGroupID<0)
	{
		newGroupWidget=new ASICTableWidget(gMainWin);
		newGroupWidget->Title=ui->title->text();
		newGroupWidget->Description=ui->description->toPlainText();
		newGroupWidget->UserName=ui->username->text();
		newGroupWidget->Password=ui->password->text();
		newGroupWidget->APIPort=static_cast<quint16>(ui->apiport->text().toUInt());
		newGroupWidget->WebPort=static_cast<quint16>(ui->webport->text().toUInt());
		emit(newGroupCreated(newGroupWidget));
		return;
	}
	ASICTableWidget *existingGroup=gMainWin->GroupTabsWidgets->at(pGroupID);
}

void GroupSettingsDialog::on_buttonBox_rejected()
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	pGroupID=-1;
}

