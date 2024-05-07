#include "asictablewidget.h"
#include "logger.hpp"
#include "main.hpp"

ASICTableWidget::ASICTableWidget(QWidget *parent) : QTableWidget(parent)
{
	gLogger->Log("ASICTableWidget::"+string(__FUNCTION__), LOG_DEBUG);
    WebPort=0;
    APIPort=0;
    GroupID=0;
    DeviceList=new QVector <ASICDevice *>;
    connect(this, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(on_cellDoubleClicked(int,int)));
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
    deviceToAdd->UserName=this->UserName;
    deviceToAdd->Password=this->Password;
    deviceToAdd->APIPort=this->APIPort;
    deviceToAdd->WebPort=this->WebPort;
    deviceToAdd->GroupID=this->GroupID;
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
