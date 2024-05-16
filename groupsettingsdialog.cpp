#include "groupsettingsdialog.h"
#include "ui_groupsettingsdialog.h"
#include "logger.hpp"

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

void GroupSettingsDialog::showGroupSettings(ASICTableWidget *group_widget)
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	if(group_widget==nullptr)
	{
		gLogger->Log("incorrect group_widget=nullptr", LOG_ERR);
		pGroupWidget=nullptr;
		ui->title->setText(QString("New group"));
	}
	else
	{
		pGroupWidget=group_widget;
		ui->title->setText(pGroupWidget->Title());
		ui->description->setText(pGroupWidget->Description());
		ui->username->setText(pGroupWidget->UserName());
		ui->password->setText(pGroupWidget->Password());
		ui->apiport->setText(QString::number(pGroupWidget->APIPort()));
		ui->webport->setText(QString::number(pGroupWidget->WebPort()));
	}
	this->show();
}

void GroupSettingsDialog::on_buttonBox_accepted()
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	bool isANewGroup=!pGroupWidget;
	if(isANewGroup)
	{
		pGroupWidget=new ASICTableWidget(this);
	}
	pGroupWidget->SetTitle(ui->title->text());
	pGroupWidget->SetDescription(ui->description->toPlainText());
	pGroupWidget->SetUserName(ui->username->text());
	pGroupWidget->SetPassword(ui->password->text());
	pGroupWidget->SetAPIPort(static_cast<quint16>(ui->apiport->text().toUInt()));
	pGroupWidget->SetWebPort(static_cast<quint16>(ui->webport->text().toUInt()));
	if(isANewGroup)
	{
		emit(newGroupCreated(pGroupWidget));
	}
	pGroupWidget=nullptr;
}

void GroupSettingsDialog::on_buttonBox_rejected()
{
	gLogger->Log("GroupSettingsDialog::"+string(__FUNCTION__), LOG_DEBUG);
	ui->title->setText(QString("New group"));
	pGroupWidget=nullptr;
}

