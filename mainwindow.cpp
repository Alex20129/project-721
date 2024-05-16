#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "configurationholder.h"
#include "logger.hpp"
#include "main.hpp"

MainWindow *gMainWin=nullptr;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ActiveUploadingThreads=0;

	firmwareData=new(QByteArray);
	ColumnTitles=new QStringList({"Address", "Type", "Miner", "HashRate", "Temperature", "Frequency", "Uptime", "Hardware Errors", "Pool", "User"});

	GroupTabsWidgets=new QVector <ASICTableWidget *>;

	RefreshTimer=new(QTimer);
	RefreshTimer->setInterval(DEFAULT_UPDATE_INTERVAL);
	connect(RefreshTimer, &QTimer::timeout, this, &MainWindow::rescanDevices);

	SleepWakeTimer=new(QTimer);
	SleepWakeTimer->setInterval(2000);
	connect(SleepWakeTimer, SIGNAL(timeout()), this, SLOT(sleepOrWake()));

	IsAwake=true;

	ui->setupUi(this);

	connect(this, SIGNAL(FirmwareUploadProgess(int)), ui->progressBar, SLOT(setValue(int)));
	connect(this, SIGNAL(timeToSleep()), this, SLOT(on_timeToSleep()));
	connect(this, SIGNAL(timeToWakeUp()), this, SLOT(on_timeToWakeUp()));

	connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateDeviceView()));

	setWindowTitle(QString(PROGRAM_NAME)+QString(" ")+QString(PROGRAM_VERSION));

	if(!gMainWin)
	{
		gMainWin=this;
	}
}

MainWindow::~MainWindow()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	delete(RefreshTimer);
	delete ui;
}

void MainWindow::sleepOrWake()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(QTime::currentTime()>gAppConfig->TimeToSleep &&
	   QTime::currentTime()<gAppConfig->TimeToWakeUp)
	{
		emit(timeToSleep());
	}
	else
	{
		emit(timeToWakeUp());//and dig myself out of this hell
	}
}

void MainWindow::on_timeToSleep()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(!IsAwake)
	{
		return;
	}
	IsAwake=false;

	this->RefreshTimer->stop();
	this->SleepWakeTimer->stop();

	for(int device=0; device<GroupTabsWidgets->first()->DeviceList->size(); device++)
	{
		GroupTabsWidgets->first()->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/kill_bmminer1.cgi");
	}

	if(ui->updateButton->isFlat())
	{
		RefreshTimer->start();
	}
	if(gAppConfig->TimeToSleep!=gAppConfig->TimeToWakeUp)
	{
		SleepWakeTimer->start();
	}
}

void MainWindow::on_timeToWakeUp()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(IsAwake)
	{
		return;
	}
	IsAwake=true;

	this->RefreshTimer->stop();
	this->SleepWakeTimer->stop();

	for(int device=0; device<GroupTabsWidgets->first()->DeviceList->size(); device++)
	{
		GroupTabsWidgets->first()->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/start_bmminer.cgi");
	}

	if(ui->updateButton->isFlat())
	{
		RefreshTimer->start();
	}
	if(gAppConfig->TimeToSleep!=gAppConfig->TimeToWakeUp)
	{
		SleepWakeTimer->start();
	}
}

void MainWindow::loadTabs()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *newGroupWidget;
	int groupsCount, group;
	QJsonArray groupsJSONArray;
	if(gAppConfig->JSONData.value("groups").isArray())
	{
		groupsJSONArray=gAppConfig->JSONData.value("groups").toArray();
	}
	groupsCount=groupsJSONArray.size();
	if(groupsCount<1)
	{
		emit(NeedToCreateNewGroup());
		return;
	}
	for(group=0; group<groupsCount; group++)
	{
		QJsonObject groupJSONObject=groupsJSONArray.at(group).toObject();
		newGroupWidget=new ASICTableWidget(this);
		newGroupWidget->SetTitle(groupJSONObject.value("title").toString());
		newGroupWidget->SetDescription(groupJSONObject.value("description").toString());
		newGroupWidget->SetUserName(groupJSONObject.value("username").toString());
		newGroupWidget->SetPassword(groupJSONObject.value("password").toString());
		newGroupWidget->SetAPIPort(groupJSONObject.value("apiport").toInt());
		newGroupWidget->SetWebPort(groupJSONObject.value("webport").toInt());
		addNewGroup(newGroupWidget);
	}
	int devicesCount, device;
	QJsonArray devicesJSONArray;
	if(gAppConfig->JSONData.value("devices").isArray())
	{
		devicesJSONArray=gAppConfig->JSONData.value("devices").toArray();
	}
	devicesCount=devicesJSONArray.size();
	for(device=0; device<devicesCount; device++)
	{
		int device_group=devicesJSONArray.at(device).toObject().value("group").toInt();
		ASICDevice *newDevice=new(ASICDevice);
		newDevice->Address=QHostAddress(devicesJSONArray.at(device).toObject().value("address").toString());
		GroupTabsWidgets->first()->addDevice(newDevice);
		if(device_group>0 && device_group<GroupTabsWidgets->count())
		{
			ASICTableWidget *tw=GroupTabsWidgets->at(device_group);
			tw->addDevice(newDevice);
		}
	}
	return;
}

void MainWindow::saveTabs()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	int tab, device;
	gAppConfig->JSONData.remove("groups");
	gAppConfig->JSONData.remove("devices");
	QJsonArray Groups, Devices;
	for(tab=0; tab<GroupTabsWidgets->size(); tab++)
	{
		ASICTableWidget *TabWidget=GroupTabsWidgets->at(tab);
		QJsonObject TabObject;
		TabObject.insert("title", TabWidget->Title());
		TabObject.insert("description", TabWidget->Description());
		TabObject.insert("username", TabWidget->UserName());
		TabObject.insert("password", TabWidget->Password());
		TabObject.insert("apiport", QJsonValue::fromVariant(TabWidget->APIPort()));
		TabObject.insert("webport", QJsonValue::fromVariant(TabWidget->WebPort()));
		Groups.append(TabObject);
		gAppConfig->JSONData.insert("groups", Groups);
	}
	for(device=0; device<DefaultDeviceList->size(); device++)
	{
		QJsonObject DeviceObject;
		DeviceObject.insert("address", DefaultDeviceList->at(device)->Address.toString());
		DeviceObject.insert("description", DefaultDeviceList->at(device)->Description);
		DeviceObject.insert("miner", DefaultDeviceList->at(device)->Miner);
		DeviceObject.insert("type", DefaultDeviceList->at(device)->Type);
		DeviceObject.insert("group", QJsonValue::fromVariant(DefaultDeviceList->at(device)->GroupID));
		Devices.append(DeviceObject);
		gAppConfig->JSONData.insert("devices", Devices);
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(event->isAccepted())
	{
		saveTabs();
		QApplication::exit(0);
	}
}

void MainWindow::rescanDevices()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	emit(NeedToRescanDevices(DefaultDeviceList));
}

void MainWindow::updateDeviceView()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *catw=GroupTabsWidgets->at(ui->tabWidget->currentIndex());
	if(!catw)
	{
		return;
	}
	if(catw->DeviceList->isEmpty())
	{
		return;
	}
	double tHashRate=0;
	catw->setSortingEnabled(false);
	for(int row=0; row<catw->rowCount(); row++)
	{
		tHashRate+=catw->DeviceList->at(row)->HashRate();
		QTableWidgetItem **item;
		item=new QTableWidgetItem *[catw->columnCount()];
		//===================================
		item[0]=new QTableWidgetIPItem;
		item[0]->setData(Qt::DisplayRole, catw->DeviceList->at(row)->Address.toIPv4Address());
		item[0]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		/*
		if(catw->DeviceList->at(row)->IsAlarmed())
		{
			item[0]->setIcon(QIcon(":/images/alarm-128.png"));
		}
		else
		{
			item[0]->setIcon(QIcon());
			//item1->setIcon(QIcon(":/images/lag1-128.png"));
		}
		*/
		for(int i=1; i<catw->columnCount(); i++)
		{
			item[i]=new QTableWidgetItem;
		}
		if(catw->DeviceList->at(row)->Type.isEmpty())
		{
			item[1]->setData(Qt::DisplayRole, QString("unknown"));
		}
		else
		{
			item[1]->setData(Qt::DisplayRole, catw->DeviceList->at(row)->Type);
		}
		item[1]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		if(catw->DeviceList->at(row)->Miner.isEmpty())
		{
			item[2]->setData(Qt::DisplayRole, QString("unknown"));
		}
		else
		{
			item[2]->setData(Qt::DisplayRole, catw->DeviceList->at(row)->Miner);
		}
		item[2]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		int iHashRate=static_cast<int>(catw->DeviceList->at(row)->HashRate());
		item[3]->setData(Qt::DisplayRole, iHashRate);
		item[3]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		int iTemp=static_cast<int>(catw->DeviceList->at(row)->Temperature());
		item[4]->setData(Qt::DisplayRole, iTemp);
		item[4]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		item[5]->setData(Qt::DisplayRole, catw->DeviceList->at(row)->Frequency());
		item[5]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		const qint64 DAY=86400;
		uint UT=catw->DeviceList->at(row)->UpTime();
		qint64 days=UT/DAY;
		QTime t=QTime(0,0).addSecs(UT % DAY);
		QString uptimeStr;
		if(days>0)
		{
			uptimeStr+=QString("%1d ").arg(days);
		}
		if(t.hour()>0)
		{
			uptimeStr+=QString("%1h ").arg(t.hour());
		}
		if(t.minute()>0)
		{
			uptimeStr+=QString("%1m ").arg(t.minute());
		}
		uptimeStr+=QString("%1s").arg(t.second());

		item[6]->setData(Qt::DisplayRole, uptimeStr);
		item[6]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		item[7]->setData(Qt::DisplayRole, catw->DeviceList->at(row)->HardwareErrors());
		item[7]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		item[8]->setData(Qt::DisplayRole, catw->DeviceList->at(row)->ActivePool()->URL.host());
		item[8]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		item[9]->setData(Qt::DisplayRole, catw->DeviceList->at(row)->ActivePool()->User);
		item[9]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		for(int column=0; column<catw->columnCount(); column++)
		{
			if(catw->item(row, column)!=nullptr)
			{
				delete(catw->item(row, column));
			}
		}
		for(int column=0; column<catw->columnCount(); column++)
		{
			catw->setItem(row, column, item[column]);
		}
	}
	catw->setSortingEnabled(true);
	uint iHashRate=static_cast<unsigned int>(tHashRate/1000.0);
	ui->totalHashrate->setText(QString::number(iHashRate));
	ui->totalDevices->setText(QString::number(catw->rowCount()));
}

void MainWindow::applyGroupSettings(ASICTableWidget *group_widget)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ui->tabWidget->setTabText(group_widget->GroupID(), group_widget->Title());
}

void MainWindow::on_updateButton_clicked()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	RefreshTimer->setInterval(gAppConfig->UpdateInterval);
	if(ui->updateButton->isFlat())
	{
		ui->updateButton->setFlat(0);
		RefreshTimer->stop();
	}
	else
	{
		ui->updateButton->setFlat(1);
		RefreshTimer->start();
	}
}

void MainWindow::addNewDevices(QString addressFrom, QString addressTo)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	QHostAddress AddrFrom(addressFrom), AddrTo(addressTo);
	ASICDevice *newDevice;
	if(AddrTo.toIPv4Address()>AddrFrom.toIPv4Address())
	{
		for(quint32 address=AddrFrom.toIPv4Address(); address<=AddrTo.toIPv4Address(); address++)
		{
			newDevice=new(ASICDevice);
			newDevice->Address=QHostAddress(address);
			GroupTabsWidgets->first()->addDevice(newDevice);
		}
		return;
	}
	newDevice=new(ASICDevice);
	newDevice->Address=AddrFrom;
	GroupTabsWidgets->first()->addDevice(newDevice);
	updateDeviceView();
}

void MainWindow::addNewGroup(ASICTableWidget *new_group_widget)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(!new_group_widget)
	{
		return;
	}
	new_group_widget->SetGroupID(GroupTabsWidgets->size());

	new_group_widget->setSortingEnabled(true);
	new_group_widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	new_group_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	new_group_widget->setSelectionBehavior(QAbstractItemView::SelectRows);
	new_group_widget->setWordWrap(false);
	new_group_widget->setShowGrid(false);
	new_group_widget->setIconSize(QSize(32, 32));

	new_group_widget->setColumnCount(ColumnTitles->count());
	new_group_widget->setHorizontalHeaderLabels(*ColumnTitles);

	new_group_widget->horizontalHeader()->setSectionResizeMode(QHeaderView::QHeaderView::Interactive);
	//qweWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	//qweWidget->horizontalHeader()->setSectionResizeMode(qweWidget->horizontalHeader()->count()-1, QHeaderView::Stretch);
	new_group_widget->verticalHeader()->setHidden(true);

	new_group_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(new_group_widget, SIGNAL(customContextMenuRequested(QPoint)),  this, SLOT(on_customContextMenuRequested(QPoint)));

	if(0==new_group_widget->GroupID())
	{
		DefaultDeviceList=new_group_widget->DeviceList;
	}
	GroupTabsWidgets->append(new_group_widget);
	ui->tabWidget->addTab(new_group_widget, new_group_widget->Title());

	QAction *qweAction=new QAction(new_group_widget->Title(), new_group_widget);
	ui->menuMove_devices_to->addAction(qweAction);
	connect(qweAction, SIGNAL(triggered(bool)), this, SLOT(addDevicesToGroup()));
}

void MainWindow::on_rebootButton_clicked()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=GroupTabsWidgets->at(ui->tabWidget->currentIndex());
	QStringList HostsToReboot;
	ASICDevice *PickedDevice;
	for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
	{
		HostsToReboot.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
	}
	if(HostsToReboot.isEmpty())
	{
		gLogger->Log("Host list is empty =/", LOG_NOTICE);
		return;
	}
	for(int host=0; host<HostsToReboot.count(); host++)
	{
		for(int device=0; device<GroupTabsWidgets->first()->DeviceList->size(); device++)
		{
			if(HostsToReboot.at(host)==GroupTabsWidgets->first()->DeviceList->at(device)->Address.toString())
			{
				PickedDevice=GroupTabsWidgets->first()->DeviceList->at(device);
				break;
			}
		}
		if(!PickedDevice)
		{
			gLogger->Log("cannot find the device with address "+HostsToReboot.at(host).toStdString(), LOG_INFO);
			gLogger->Log("continue...", LOG_INFO);
			continue;
		}
		gLogger->Log("now reboot device "+PickedDevice->Address.toString().toStdString(), LOG_INFO);
		PickedDevice->ExecCGIScriptViaGET("/cgi-bin/reboot.cgi");
	}
}

void MainWindow::addDevice(ASICDevice *device)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(GroupTabsWidgets->isEmpty())
	{
		return;
	}
	GroupTabsWidgets->first()->addDevice(device);
}

void MainWindow::addDevicesToGroup()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=GroupTabsWidgets->at(ui->tabWidget->currentIndex());
	QAction *senderAction=qobject_cast<QAction *>(sender());
	ASICTableWidget *senderParentWidget=qobject_cast<ASICTableWidget *>(senderAction->parent());

	QStringList HostsToAdd;
	for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
	{
		HostsToAdd.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
	}
	if(HostsToAdd.isEmpty())
	{
		gLogger->Log("Host list is empty =/", LOG_NOTICE);
		return;
	}
	for(int host=0; host<HostsToAdd.count(); host++)
	{
		for(int i=0; i<GroupTabsWidgets->first()->DeviceList->size(); i++)
		{
			if(HostsToAdd.at(host)==GroupTabsWidgets->first()->DeviceList->at(i)->Address.toString())
			{
				senderParentWidget->addDevice(GroupTabsWidgets->first()->DeviceList->at(i));
				break;
			}
		}
	}
}

void MainWindow::on_customContextMenuRequested(QPoint position)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	Q_UNUSED(position)
	ui->menuControl->exec(QCursor::pos());
}

void MainWindow::on_firmwareButton_clicked()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	QFile *firmwareFile;
	firmwareFile=new QFile(QFileDialog::getOpenFileName(this, "Choose firmware...", "", "Tar-GZip archive (*.tar.gz);; Tar-BZip2 archive (*.tar.bz2)"));
	if(firmwareFile->fileName().isEmpty())
	{
		gLogger->Log("Filename is empty =/", LOG_NOTICE);
		return;
	}
	if(!firmwareFile->open(QIODevice::ReadOnly))
	{
		gLogger->Log("Cannot open the file =/", LOG_ERR);
		gLogger->Log(firmwareFile->errorString().toStdString(), LOG_ERR);
		return;
	}
	*firmwareData=firmwareFile->readAll();

	firmwareFile->close();
	gLogger->Log("Firmware has been loaded from "+firmwareFile->fileName().toStdString(), LOG_INFO);
	delete(firmwareFile);

	ASICTableWidget *ctw=GroupTabsWidgets->at(ui->tabWidget->currentIndex());
	QStringList HostsToFlash;
	for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
	{
		HostsToFlash.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
	}
	if(HostsToFlash.isEmpty())
	{
		gLogger->Log("Host list is empty =/", LOG_NOTICE);
		return;
	}
	ui->progressBar->setVisible(true);
	RefreshTimer->stop();
	ui->rebootButton->setFlat(0);
	int host;
	for(host=0; host<HostsToFlash.count(); host++)
	{
		while(ActiveUploadingThreads>=gAppConfig->ActiveThreadsMaxNum)
		{
			QApplication::processEvents();
		}
		for(int device=0; device<GroupTabsWidgets->first()->DeviceList->size(); device++)
		{
			if(HostsToFlash.at(host)==GroupTabsWidgets->first()->DeviceList->at(device)->Address.toString())
			{
				uploadFirmware(GroupTabsWidgets->first()->DeviceList->at(device));
				break;
			}
		}
		emit(FirmwareUploadProgess(host*100/HostsToFlash.count()));
	}
	emit(FirmwareUploadProgess(host*100/HostsToFlash.count()));
}

void MainWindow::uploadFirmware(ASICDevice *device)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	QUrl deviceURL;
	deviceURL.setScheme("http");
	deviceURL.setHost(device->Address.toString());
	deviceURL.setPort(device->WebPort);
	deviceURL.setUserName(device->UserName);
	deviceURL.setPassword(device->Password);
	if(gAppConfig->ClearUpSettingsWhenFirmwareUpdate)
	{
		deviceURL.setPath("/cgi-bin/upgrade_clear.cgi");
		gLogger->Log("upgrade_clear.cgi", LOG_NOTICE);
	}
	else
	{
		deviceURL.setPath("/cgi-bin/upgrade.cgi");
		gLogger->Log("upgrade.cgi", LOG_NOTICE);
	}
	;
	gLogger->Log("upload firmware on "+deviceURL.host().toStdString(), LOG_NOTICE);
	QNetworkRequest flashRequest;
	flashRequest.setUrl(deviceURL);
	flashRequest.setHeader(QNetworkRequest::UserAgentHeader, API_HEADER_USER_AGENT);
	QHttpMultiPart *mpData=new QHttpMultiPart();
	QHttpPart firmwarePart;
	firmwarePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
	firmwarePart.setHeader(QNetworkRequest::ContentDispositionHeader, "multipart/form-data; name=\"datafile\"; filename=\"firmware.tar.gz\"");
	firmwarePart.setBody(*firmwareData);
	mpData->setContentType(QHttpMultiPart::FormDataType);
	mpData->append(firmwarePart);
	QNetworkAccessManager *netManager=new(QNetworkAccessManager);
	connect(netManager, SIGNAL(authenticationRequired(QNetworkReply *,QAuthenticator *)), this, SLOT(authenticationHandler(QNetworkReply *, QAuthenticator *)));
	connect(netManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(uploadFirmwareFinished(QNetworkReply *)));
	QNetworkReply *reply=netManager->post(flashRequest, mpData);
	mpData->setParent(reply);
}

void MainWindow::authenticationHandler(QNetworkReply *repl, QAuthenticator *auth)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(repl->url().toString().toStdString(), LOG_DEBUG);
	gLogger->Log(auth->user().toStdString(), LOG_DEBUG);
	gLogger->Log(auth->password().toStdString(), LOG_DEBUG);
}

void MainWindow::uploadFirmwareFinished(QNetworkReply *reply)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ActiveUploadingThreads--;
	QByteArray ReceivedData;
	if(reply->isReadable())
	{
		ReceivedData=reply->readAll();
	}
	if(reply->error()==QNetworkReply::NoError)
	{
		gLogger->Log("uploadFirmware reply success", LOG_INFO);
	}
	else
	{
		gLogger->Log("uploadFirmware reply error: "+reply->errorString().toStdString(), LOG_ERR);
	}
	gLogger->Log(ReceivedData.toStdString(), LOG_DEBUG);
	reply->manager()->disconnect();
	reply->manager()->deleteLater();
	for(int device=0; device<GroupTabsWidgets->first()->DeviceList->size(); device++)
	{
		if(reply->request().url().host()==GroupTabsWidgets->first()->DeviceList->at(device)->Address.toString())
		{
			gLogger->Log("now reboot device "+reply->request().url().host().toStdString(), LOG_INFO);
			GroupTabsWidgets->first()->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/reboot.cgi");
			break;
		}
	}
	if(!ActiveUploadingThreads)
	{
		if(ui->progressBar->value()==100)
		{
			ui->progressBar->setVisible(false);
		}
	}
}

void MainWindow::on_actionReset_to_default_triggered()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=GroupTabsWidgets->at(ui->tabWidget->currentIndex());
	QStringList HostsToReset;
	for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
	{
		HostsToReset.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
	}
	if(HostsToReset.isEmpty())
	{
		gLogger->Log("Host list is empty =/", LOG_NOTICE);
		return;
	}
	for(int host=0; host<HostsToReset.count(); host++)
	{
		for(int device=0; device<GroupTabsWidgets->first()->DeviceList->size(); device++)
		{
			if(HostsToReset.at(host)==GroupTabsWidgets->first()->DeviceList->at(device)->Address.toString())
			{
				gLogger->Log("now reset device "+HostsToReset.at(host).toStdString(), LOG_INFO);
				GroupTabsWidgets->first()->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/reset_conf.cgi");
				break;
			}
		}
	}
}

void MainWindow::on_actionToggle_fullscreen_triggered()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(gMainWin->isFullScreen())
	{
		gMainWin->showNormal();
	}
	else
	{
		gMainWin->showFullScreen();
	}
}

void MainWindow::on_actionGroup_summary_triggered()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(ui->groupSummary->isVisible())
	{
		ui->groupSummary->setVisible(0);
	}
	else
	{
		ui->groupSummary->setVisible(1);
	}
}

void MainWindow::on_actionRemove_devices_from_group_triggered()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=GroupTabsWidgets->at(ui->tabWidget->currentIndex());
	QStringList HostsToRemove;
	for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
	{
		HostsToRemove.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
	}
	if(HostsToRemove.isEmpty())
	{
		gLogger->Log("Host list is empty =/", LOG_NOTICE);
		return;
	}

	for(int host=0; host<HostsToRemove.count(); host++)
	{
		for(int device=0; device<GroupTabsWidgets->first()->DeviceList->size(); device++)
		{
			if(HostsToRemove.at(host)==GroupTabsWidgets->first()->DeviceList->at(device)->Address.toString())
			{
				gLogger->Log("now remove the device "+HostsToRemove.at(host).toStdString(), LOG_INFO);
				ASICDevice *deviceToRemove=GroupTabsWidgets->first()->DeviceList->at(device);
				if(ctw==GroupTabsWidgets->first())
				{
					for(int tab=0; tab<GroupTabsWidgets->size(); tab++)
					{
						ASICTableWidget *tw=GroupTabsWidgets->at(tab);
						tw->removeDevice(deviceToRemove);
					}
					delete deviceToRemove;
					break;
				}
				else
				{
					ctw->removeDevice(deviceToRemove);
					break;
				}
			}
		}
	}
	updateDeviceView();
}

void MainWindow::uploadSettings(QStringList settings)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=GroupTabsWidgets->at(ui->tabWidget->currentIndex());
	QStringList HostsToSetup;
	QByteArray DataToSend;
	ASICDevice *PickedDevice;
	QJsonObject SettingsToSendJSON;
	QJsonArray PoolsToSendJSON;

	SettingsToSendJSON.insert("api-groups", QString("A:stats:pools:devs:summary:version"));
	SettingsToSendJSON.insert("api-network", QJsonValue::fromVariant(true));
	SettingsToSendJSON.insert("bitmain-use-vil", QJsonValue::fromVariant(true));
	SettingsToSendJSON.insert("multi-version", QJsonValue::fromVariant(1));

	for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
	{
		HostsToSetup.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
	}
	if(HostsToSetup.isEmpty())
	{
		gLogger->Log("Host list is empty =/", LOG_NOTICE);
		return;
	}
	int setting;
	QByteArray setting_utf8;
	for(int host=0; host<HostsToSetup.size(); host++)
	{
		PickedDevice=nullptr;
		DataToSend.clear();
		if(SettingsToSendJSON.value("pools").isArray())
		{
			SettingsToSendJSON.remove("pools");
		}
		for(int device=0; device<GroupTabsWidgets->first()->DeviceList->size(); device++)
		{
			if(HostsToSetup.at(host)==GroupTabsWidgets->first()->DeviceList->at(device)->Address.toString())
			{
				PickedDevice=GroupTabsWidgets->first()->DeviceList->at(device);
				break;
			}
		}
		if(!PickedDevice)
		{
			gLogger->Log("cannot find the device with address "+HostsToSetup.at(host).toStdString(), LOG_INFO);
			continue;
		}

		for(setting=0; setting<settings.size(); setting++)
		{
			if(gAppConfig->AddressBasedPostfixToWorkerName)
			{
				if(settings.at(setting).split('=').first()==QString("pool1user"))
				{
					setting_utf8=QString("_ant_pool1user=").toUtf8();
					if(2==settings.at(setting).split('=', QString::SkipEmptyParts).count())
					{
						setting_utf8+=(settings.at(setting).split('=').last()+
									  QString(".")+
									  HostsToSetup.at(host).split('.').at(2)+
									  QString("x")+
									  HostsToSetup.at(host).split('.').at(3)).toUtf8();
					}
					setting_utf8+=QString(" ").toUtf8();
				}
				else if(settings.at(setting).split('=').first()==QString("pool2user"))
				{
					setting_utf8=QString("_ant_pool2user=").toUtf8();
					if(2==settings.at(setting).split('=', QString::SkipEmptyParts).count())
					{
						setting_utf8+=(settings.at(setting).split('=').last()+
									  QString(".")+
									  HostsToSetup.at(host).split('.').at(2)+
									  QString("x")+
									  HostsToSetup.at(host).split('.').at(3)).toUtf8();
					}
					setting_utf8+=QString(" ").toUtf8();
				}
				else if(settings.at(setting).split('=').first()==QString("pool3user"))
				{
					setting_utf8=QString("_ant_pool3user=").toUtf8();
					if(2==settings.at(setting).split('=', QString::SkipEmptyParts).count())
					{
						setting_utf8+=(settings.at(setting).split('=').last()+
									  QString(".")+
									  HostsToSetup.at(host).split('.').at(2)+
									  QString("x")+
									  HostsToSetup.at(host).split('.').at(3)).toUtf8();
					}
					setting_utf8+=QString(" ").toUtf8();
				}
				else
				{
					setting_utf8=(QString("_ant_")+settings.at(setting)+QString(" ")).toUtf8();
				}
			}
			else
			{
				setting_utf8=(QString("_ant_")+settings.at(setting)+QString(" ")).toUtf8();
			}
			DataToSend+=setting_utf8;
		}

		gLogger->Log(PickedDevice->Address.toString().toStdString()+" will be configured now", LOG_INFO);
		PickedDevice->UploadDataViaPOST("/cgi-bin/set_miner_conf.cgi", &DataToSend);
		PickedDevice->ExecCGIScriptViaGET("/cgi-bin/reboot.cgi");
	}
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ui->tabWidget->widget(index)->deleteLater();
	GroupTabsWidgets->at(index)->deleteLater();
	GroupTabsWidgets->removeAt(index);
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	emit(NeedToShowGroupSettings(GroupTabsWidgets->at(index)));
}

void MainWindow::on_actionSupport_website_triggered()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	QUrl SupportOnSite("https://www.google.com");
	QDesktopServices::openUrl(SupportOnSite);
}

void MainWindow::on_actionBasic_settings_triggered()
{
	emit(NeedToShowBasicSettingsWindow());
}

void MainWindow::on_actionNetwork_settings_triggered()
{
	emit(NeedToShowNetworkSettingsWindow());
}

void MainWindow::on_actionDevice_settings_triggered()
{
	emit(NeedToShowDeviceSettingsWindow());
}

void MainWindow::on_actionFind_devices_triggered()
{
	emit(NeedToShowScannerWindow());
}

void MainWindow::on_actionSleep_settings_triggered()
{
	emit(NeedToShowSleepSettingsWindow());
}

void MainWindow::on_actionAdd_devices_triggered()
{
	emit(NeedToShowAddNewDeviceDialog());
}

void MainWindow::on_actionAdd_group_triggered()
{
	emit(NeedToCreateNewGroup());
}

void MainWindow::on_deviceSettingsButton_clicked()
{
	emit(NeedToShowDeviceSettingsWindow());
}

void MainWindow::on_searchButton_clicked()
{
	emit(NeedToShowScannerWindow());
}


