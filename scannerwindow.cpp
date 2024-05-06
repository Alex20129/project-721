#include "scannerwindow.h"
#include "logger.hpp"
#include "ui_scannerwindow.h"
#include "main.hpp"

ScannerWindow::ScannerWindow(QWidget *parent) : QWidget(parent), ui(new Ui::ScannerWindow)
{
    _pIsBusy=false;
    ui->setupUi(this);

    connect(this, SIGNAL(ScanIsDone()), this, SLOT(on_scanIsDone()));
    connect(this, SIGNAL(ScanIsRun()), this, SLOT(on_scanIsRun()));

    for(int i=0; i<QNetworkInterface::allAddresses().count(); i++)
    {
        QHostAddress addr=QNetworkInterface::allAddresses().at(i);
        if(!addr.isLoopback() && !addr.isNull())
        {
            ui->knownIPcomboBox->addItem(addr.toString());
        }
    }
}

ScannerWindow::~ScannerWindow()
{
    delete ui;
}

void ScannerWindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Escape:
        {
            this->hide();
        }break;
    }
}

void ScannerWindow::updateDeviceList(ASICDevice *device)
{
    gAppLogger->Log("ScannerWindow::updateDeviceList()", LOG_DEBUG);
    gAppLogger->Log(device->Address.toString());
    disconnect(device, nullptr, nullptr, nullptr);
    mw->DefaultTabWidget->addDevice(device);
    ui->ipList->addItem(device->Address.toString());
}

void ScannerWindow::clearUpDeviceList(ASICDevice *device)
{
    gAppLogger->Log("ScannerWindow::clearUpDeviceList()", LOG_DEBUG);
    gAppLogger->Log(device->Address.toString());
    disconnect(device, nullptr, nullptr, nullptr);
    device->deleteLater();
}

void ScannerWindow::on_scanIsDone()
{
    gAppLogger->Log("ScannerWindow::on_scanIsDone()", LOG_DEBUG);
    ui->apiScanButton->setEnabled(1);
}

void ScannerWindow::on_scanIsRun()
{
    gAppLogger->Log("ScannerWindow::on_scanIsRun()", LOG_DEBUG);
    ui->apiScanButton->setEnabled(0);
}

void ScannerWindow::QuickAPIScan(QVector <ASICDevice *> devicesToCheck)
{
    gAppLogger->Log("ScannerWindow::QuickAPIScan()", LOG_DEBUG);
    if(_pIsBusy)
    {
        return;
    }
    _pIsBusy=true;
    _pStopScan=false;
    emit(ScanIsRun());
    if(devicesToCheck.isEmpty())
    {
        gAppLogger->Log("ScannerWindow::QuickAPIScan");
        gAppLogger->Log("Host list is empty =/");
        emit(ScanIsDone());
        return;
    }
    int host=0;
    ui->progressBar->setMaximum(devicesToCheck.count());
    ui->progressBar->setValue(host);
    while(host<devicesToCheck.count())
    {
        while(ASICDevice::ActiveThreadsNum>gAppConfig->ActiveThreadsMaxNum)
        {
            QApplication::processEvents();
        }
        if(_pStopScan)
        {
            disconnect(devicesToCheck.at(host), nullptr, nullptr, nullptr);
            devicesToCheck.at(host)->deleteLater();
        }
        else
        {
            devicesToCheck.at(host)->SendCommand(QByteArray("summary"));
            devicesToCheck.at(host)->SendCommand(QByteArray("estats"));
        }
        host++;
        ui->progressBar->setValue(host);
    }
    while(ASICDevice::ActiveThreadsNum)
    {
        QApplication::processEvents();
    }
    _pIsBusy=false;
    emit(ScanIsDone());
}

void ScannerWindow::on_apiScanButton_clicked()
{
    ui->ipList->clear();
    QHostAddress AddrFrom(ui->ipFrom->text()), AddrTo(ui->ipTo->text());
    QVector <ASICDevice *> Hosts;
    for(quint32 address=AddrFrom.toIPv4Address(); address<=AddrTo.toIPv4Address(); address++)
    {
        ASICDevice *newDevice=new ASICDevice;
        newDevice->Address=QHostAddress(address);
        newDevice->UserName=ui->username->text();
        newDevice->Password=ui->password->text();
        newDevice->APIPort=static_cast<quint16>(ui->apiPort->text().toUInt());
        Hosts.append(newDevice);
        connect(newDevice, SIGNAL(SocketConnected(ASICDevice *)), this, SLOT(updateDeviceList(ASICDevice *)));
        connect(newDevice, SIGNAL(SocketError(ASICDevice *)), this, SLOT(clearUpDeviceList(ASICDevice *)));
    }
    QuickAPIScan(Hosts);
}

void ScannerWindow::on_stopScanButton_clicked()
{
    _pStopScan=true;
}

void ScannerWindow::on_knownIPcomboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(4!=ui->knownIPcomboBox->currentText().split(".").count())
    {
        return;
    }
    ui->ipFrom->setText(ui->knownIPcomboBox->currentText().split(".").at(0)+"."+
                        ui->knownIPcomboBox->currentText().split(".").at(1)+"."+
                        ui->knownIPcomboBox->currentText().split(".").at(2)+".2");
    ui->ipTo->setText(ui->knownIPcomboBox->currentText().split(".").at(0)+"."+
                      ui->knownIPcomboBox->currentText().split(".").at(1)+"."+
                      ui->knownIPcomboBox->currentText().split(".").at(2)+".254");
}
