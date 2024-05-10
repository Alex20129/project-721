#include "groupsettingsdialog.h"
#include "ui_groupsettingsdialog.h"
#include "logger.hpp"
#include "mainwindow.hpp"

GroupSettingsDialog::GroupSettingsDialog(QWidget *parent) : QDialog(parent),
    ui(new Ui::GroupSettingsDialog)
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	ui->setupUi(this);
	pGroupWidget=nullptr;
	ui->title->setText(QString("New group"));
}

GroupSettingsDialog::~GroupSettingsDialog()
{
    delete ui;
}

void GroupSettingsDialog::showGroupSettings(int group_id)
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	if(group_id<0)
	{
		gLogger->Log("incorrect group_id="+to_string(group_id), LOG_ERR);
		return;
	}
	pGroupWidget=gMainWin->GroupTabsWidgets->at(group_id);
	ui->title->setText(pGroupWidget->Title);
	ui->description->setText(pGroupWidget->Description);
	ui->username->setText(pGroupWidget->UserName);
	ui->password->setText(pGroupWidget->Password);
	ui->apiport->setText(QString::number(pGroupWidget->APIPort));
	ui->webport->setText(QString::number(pGroupWidget->WebPort));
	this->show();
}

void GroupSettingsDialog::on_buttonBox_accepted()
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	bool isANewGroup=!pGroupWidget;
	if(isANewGroup)
	{
		pGroupWidget=new ASICTableWidget(gMainWin);
	}
	pGroupWidget->Title=ui->title->text();
	pGroupWidget->Description=ui->description->toPlainText();
	pGroupWidget->UserName=ui->username->text();
	pGroupWidget->Password=ui->password->text();
	pGroupWidget->APIPort=static_cast<quint16>(ui->apiport->text().toUInt());
	pGroupWidget->WebPort=static_cast<quint16>(ui->webport->text().toUInt());
	if(isANewGroup)
	{
		emit(newGroupCreated(pGroupWidget));
		pGroupWidget=nullptr;
	}
	else
	{
		emit(groupSettingsUpdated(pGroupWidget));
		pGroupWidget=nullptr;
	}
}

void GroupSettingsDialog::on_buttonBox_rejected()
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	ui->title->setText(QString("New group"));
	pGroupWidget=nullptr;
}

