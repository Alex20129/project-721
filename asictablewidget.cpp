#include "asictablewidget.h"
#include "logger.hpp"
#include "main.hpp"

ASICTableWidget::ASICTableWidget(QWidget *parent) : QTableWidget(parent)
{
	gLogger->Log("ASICTableWidget::"+string(__FUNCTION__), LOG_DEBUG);
	pWebPort=0;
	pAPIPort=0;
	pGroupID=0;
    DeviceList=new QVector <ASICDevice *>;

	connect(this, &ASICTableWidget::cellDoubleClicked, this, &ASICTableWidget::on_cellDoubleClicked);
	connect(this, &ASICTableWidget::GroupSettingsHaveBeenChanged, this, &ASICTableWidget::ApplyGroupSettingsToAllDevices);
}

quint16 ASICTableWidget::WebPort()
{
	return(pWebPort);
}

quint16 ASICTableWidget::APIPort()
{
	return(pAPIPort);
}

uint ASICTableWidget::GroupID()
{
	return(pGroupID);
}

QString ASICTableWidget::Title()
{
	return(pTitle);
}

QString ASICTableWidget::Description()
{
	return(pDescription);
}

QString ASICTableWidget::UserName()
{
	return(pUserName);
}

QString ASICTableWidget::Password()
{
	return(pPassword);
}

void ASICTableWidget::SetWebPort(quint16 port)
{
	pWebPort=port;
	emit(GroupSettingsHaveBeenChanged());
}

void ASICTableWidget::SetAPIPort(quint16 port)
{
	pAPIPort=port;
	emit(GroupSettingsHaveBeenChanged());
}

void ASICTableWidget::SetGroupID(uint group_id)
{
	pGroupID=group_id;
	emit(GroupSettingsHaveBeenChanged());
}

void ASICTableWidget::SetTitle(QString title)
{
	pTitle=title;
	emit(GroupSettingsHaveBeenChanged());
}

void ASICTableWidget::SetDescription(QString description)
{
	pDescription=description;
	emit(GroupSettingsHaveBeenChanged());
}

void ASICTableWidget::SetUserName(QString user_name)
{
	pUserName=user_name;
	emit(GroupSettingsHaveBeenChanged());
}

void ASICTableWidget::SetPassword(QString password)
{
	pPassword=password;
	emit(GroupSettingsHaveBeenChanged());
}

void ASICTableWidget::ApplyGroupSettingsToAllDevices()
{
	if(DeviceList->isEmpty())
	{
		return;
	}
	for(ASICDevice *device : *DeviceList)
	{
		device->WebPort=pWebPort;
		device->APIPort=pAPIPort;
		device->GroupID=pGroupID;
		device->UserName=pUserName;
		device->Password=pPassword;
	}
}

void ASICTableWidget::addDevice(ASICDevice *deviceToAdd)
{
	gLogger->Log("ASICTableWidget::"+string(__FUNCTION__), LOG_DEBUG);
    for(int device=0; device<DeviceList->count(); device++)
    {
		if(deviceToAdd->Address==DeviceList->at(device)->Address)
        {
            return;
        }
    }
	deviceToAdd->UserName=UserName();
	deviceToAdd->Password=Password();
	deviceToAdd->APIPort=APIPort();
	deviceToAdd->WebPort=WebPort();
	deviceToAdd->GroupID=GroupID();
	DeviceList->append(deviceToAdd);
    this->setRowCount(DeviceList->count());
}

void ASICTableWidget::removeDevice(ASICDevice *deviceToRemove)
{
	gLogger->Log("ASICTableWidget::"+string(__FUNCTION__), LOG_DEBUG);
    DeviceList->removeAll(deviceToRemove);
    this->setRowCount(DeviceList->count());
}

void ASICTableWidget::on_cellDoubleClicked(int row, int column)
{
	gLogger->Log("ASICTableWidget::"+string(__FUNCTION__), LOG_DEBUG);
    Q_UNUSED(row);
    if(column==0)
    {
        QUrl DeviceURL;
        DeviceURL.setScheme("http");
        DeviceURL.setHost(this->selectionModel()->selectedRows().at(0).data().toString());
        QDesktopServices::openUrl(DeviceURL);
    }
}
