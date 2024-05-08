#include "scannerwindow.h"
#include "logger.hpp"
#include "ui_scannerwindow.h"
#include "main.hpp"

ScannerWindow::ScannerWindow(QWidget *parent) : QWidget(parent), ui(new Ui::ScannerWindow)
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	pIsBusy=false;
	pHostsToScan=new QVector <ASICDevice *>;

	ui->setupUi(this);

	connect(this, SIGNAL(ScanIsDone()), this, SLOT(on_scanIsDone()));
	connect(this, SIGNAL(ScanIsRun()), this, SLOT(on_scanIsRun()));
	connect(this, SIGNAL(ScanProgress(int)), this->ui->progressBar, SLOT(setValue(int)));

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
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	delete ui;
}

void ScannerWindow::keyPressEvent(QKeyEvent *event)
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
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
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(device->Address.toString().toStdString(), LOG_DEBUG);
	disconnect(device, nullptr, nullptr, nullptr);
	mw->DefaultTabWidget->addDevice(device);
	ui->ipList->addItem(device->Address.toString());
}

void ScannerWindow::clearUpDeviceList(ASICDevice *device)
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(device->Address.toString().toStdString(), LOG_DEBUG);
	disconnect(device, nullptr, nullptr, nullptr);
	device->deleteLater();
}

void ScannerWindow::on_scanIsDone()
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	while(ASICDevice::ActiveThreadsNum)
	{
		QApplication::processEvents();
	}
	pIsBusy=false;
	ui->apiScanButton->setEnabled(1);
}

void ScannerWindow::on_scanIsRun()
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(pIsBusy)
	{
		return;
	}
	pIsBusy=true;
	ui->apiScanButton->setEnabled(0);
	QuickAPIScan(pHostsToScan);
	emit(ScanIsDone());
}

void ScannerWindow::QuickAPIScan(QVector <ASICDevice *> *devicesToCheck)
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	int progress=0;
	pStopScan=false;
	if(devicesToCheck->isEmpty())
	{
		gLogger->Log("Host list is empty =/", LOG_NOTICE);
		emit(ScanIsDone());
		return;
	}
	emit(ScanProgress(progress));
	for(ASICDevice *device : *devicesToCheck)
	{
		while(ASICDevice::ActiveThreadsNum>gAppConfig->ActiveThreadsMaxNum)
		{
			QApplication::processEvents();
		}
		if(pStopScan)
		{
			disconnect(device, nullptr, nullptr, nullptr);
			device->deleteLater();
		}
		else
		{
			device->SendCommand(QByteArray("summary"));
			device->SendCommand(QByteArray("stats"));
		}
		emit(ScanProgress(++progress));
	}
}

void ScannerWindow::on_apiScanButton_clicked()
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ui->ipList->clear();
	pHostsToScan->clear();
	QHostAddress AddrFrom(ui->ipFrom->text()), AddrTo(ui->ipTo->text());
	for(quint32 address=AddrFrom.toIPv4Address(); address<=AddrTo.toIPv4Address(); address++)
	{
		ASICDevice *newDevice=new ASICDevice;
		newDevice->Address=QHostAddress(address);
		newDevice->UserName=ui->username->text();
		newDevice->Password=ui->password->text();
		newDevice->APIPort=static_cast<quint16>(ui->apiPort->text().toUInt());
		pHostsToScan->append(newDevice);
		connect(newDevice, SIGNAL(SocketConnected(ASICDevice *)), this, SLOT(updateDeviceList(ASICDevice *)));
		connect(newDevice, SIGNAL(SocketError(ASICDevice *)), this, SLOT(clearUpDeviceList(ASICDevice *)));
	}
	ui->progressBar->setMaximum(pHostsToScan->size());
	emit(ScanIsRun());
}

void ScannerWindow::on_stopScanButton_clicked()
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
	pStopScan=true;
}

void ScannerWindow::on_knownIPcomboBox_currentIndexChanged(int index)
{
	gLogger->Log("ScannerWindow::"+string(__FUNCTION__), LOG_DEBUG);
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
