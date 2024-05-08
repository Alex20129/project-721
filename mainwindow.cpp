#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "configurationholder.h"
#include "logger.hpp"
#include "main.hpp"

MainWindow *mw;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ActiveUploadingThreads=0;
	GroupsCount=0;

	firmwareData=new(QByteArray);
	DeviceList=new(QVector <ASICDevice *>);
	ColumnTitles=new QStringList({"Address", "Type", "Miner", "HashRate", "Temperature", "Frequency", "Uptime", "Hardware Errors", "Pool", "User", "OC Profile"});

	RefreshTimer=new(QTimer);
	RefreshTimer->setInterval(DEFAULT_UPDATE_INTERVAL);
	connect(RefreshTimer, SIGNAL(timeout()), this, SLOT(rescanDevices()));
//	connect(RefreshTimer, SIGNAL(timeout()), this, SLOT(updateDeviceView()));

	SleepWakeTimer=new(QTimer);
	SleepWakeTimer->setInterval(2000);
	connect(SleepWakeTimer, SIGNAL(timeout()), this, SLOT(sleepOrWake()));

	IsAwake=true;

	ui->setupUi(this);

	connect(this, SIGNAL(FirmwareUploadProgess(int)), ui->progressBar, SLOT(setValue(int)));
	connect(this, SIGNAL(timeToSleep()), this, SLOT(on_timeToSleep()));
	connect(this, SIGNAL(timeToWakeUp()), this, SLOT(on_timeToWakeUp()));

	this->setWindowTitle(QString(PROGRAM_NAME)+QString(" ")+QString(PROGRAM_VERSION));

	if(!loadTabs())
	{
		emit(NeedToShowAddNewGroupDialog());
	}

	connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateDeviceView()));
	loadProfiles();
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

	for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
	{
		DefaultTabWidget->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/kill_bmminer1.cgi");
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

	for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
	{
		DefaultTabWidget->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/start_bmminer.cgi");
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

int MainWindow::loadTabs()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	int tabsCount=0, tab;
	if(gAppConfig->JSONData.value("groups").isArray())
	{
		tabsCount=gAppConfig->JSONData.value("groups").toArray().count();
		for(tab=0; tab<tabsCount; tab++)
		{
			addNewGroup(
			gAppConfig->JSONData.value("groups").toArray().at(tab).toObject().value("title").toString(),
			gAppConfig->JSONData.value("groups").toArray().at(tab).toObject().value("description").toString(),
			gAppConfig->JSONData.value("groups").toArray().at(tab).toObject().value("username").toString(),
			gAppConfig->JSONData.value("groups").toArray().at(tab).toObject().value("password").toString(),
			gAppConfig->JSONData.value("groups").toArray().at(tab).toObject().value("apiport").toInt(),
			gAppConfig->JSONData.value("groups").toArray().at(tab).toObject().value("webport").toInt()
			);
		}
	}
	else
	{
		return(0);
	}
	int devicesCount=0, device;
	if(gAppConfig->JSONData.value("devices").isArray())
	{
		devicesCount=gAppConfig->JSONData.value("devices").toArray().count();
		for(device=0; device<devicesCount; device++)
		{
			int group=gAppConfig->JSONData.value("devices").toArray().at(device).toObject().value("group").toInt();
			ASICDevice *newDevice=new(ASICDevice);
			newDevice->Address=QHostAddress(gAppConfig->JSONData.value("devices").toArray().at(device).toObject().value("address").toString());
			newDevice->Description=gAppConfig->JSONData.value("devices").toArray().at(device).toObject().value("description").toString();
			DefaultTabWidget->addDevice(newDevice);
			if(group<ui->tabWidget->count())
			{
				ASICTableWidget *TabWidget=qobject_cast<ASICTableWidget *>(ui->tabWidget->widget(group));
				TabWidget->addDevice(newDevice);
			}
		}
	}
	return(tabsCount);
}

void MainWindow::saveTabs()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	int tab, device;
	gAppConfig->JSONData.remove("groups");
	gAppConfig->JSONData.remove("devices");
	QJsonArray Groups, Devices;
	for(tab=0; tab<ui->tabWidget->count(); tab++)
	{
		ASICTableWidget *TabWidget=qobject_cast<ASICTableWidget *>(ui->tabWidget->widget(tab));
		QJsonObject TabObject;
		TabObject.insert("title", TabWidget->Title);
		TabObject.insert("description", TabWidget->Description);
		TabObject.insert("username", TabWidget->UserName);
		TabObject.insert("password", TabWidget->Password);
		TabObject.insert("apiport", QJsonValue::fromVariant(TabWidget->APIPort));
		TabObject.insert("webport", QJsonValue::fromVariant(TabWidget->WebPort));
		Groups.append(TabObject);
		gAppConfig->JSONData.insert("groups", Groups);
	}
	for(device=0; device<DeviceList->count(); device++)
	{
		QJsonObject DeviceObject;
		DeviceObject.insert("address", DeviceList->at(device)->Address.toString());
		DeviceObject.insert("description", DeviceList->at(device)->Description);
		DeviceObject.insert("group", QJsonValue::fromVariant(DeviceList->at(device)->GroupID));
		Devices.append(DeviceObject);
		gAppConfig->JSONData.insert("devices", Devices);
	}
}

void MainWindow::loadProfiles()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	int profilesCount, profile;

	gAppConfig->JSONData.remove("profiles_S9");
	QJsonArray Profiles_S9;

	QJsonObject Profile_S9_Object1;
	Profile_S9_Object1.insert("name", "PowerDown 9,5th");
	Profile_S9_Object1.insert("price", QJsonValue::fromVariant(19.95));
	Profiles_S9.append(Profile_S9_Object1);

	QJsonObject Profile_S9_Object2;
	Profile_S9_Object2.insert("name", "PowerDown 10,5th");
	Profile_S9_Object2.insert("price", QJsonValue::fromVariant(19.95));
	Profiles_S9.append(Profile_S9_Object2);
	
	QJsonObject Profile_S9_Object3;
	Profile_S9_Object3.insert("name", "PowerDown 12th");
	Profile_S9_Object3.insert("price", QJsonValue::fromVariant(19.95));
	Profiles_S9.append(Profile_S9_Object3);

	QJsonObject Profile_S9_Object4;
	Profile_S9_Object4.insert("name", "PowerDown 12th_LPM");
	Profile_S9_Object4.insert("price", QJsonValue::fromVariant(29.95));
	Profiles_S9.append(Profile_S9_Object4);

	QJsonObject Profile_S9_Object5;
	Profile_S9_Object5.insert("name", "PowerDown 12.5th_LPM");
	Profile_S9_Object5.insert("price", QJsonValue::fromVariant(39.95));
	Profiles_S9.append(Profile_S9_Object5);

	QJsonObject Profile_S9_Object6;
	Profile_S9_Object6.insert("name", "normal 13th_LPM");
	Profile_S9_Object6.insert("price", QJsonValue::fromVariant(49.95));
	Profiles_S9.append(Profile_S9_Object6);

	QJsonObject Profile_S9_Object7;
	Profile_S9_Object7.insert("name", "normal 13.5th");
	Profile_S9_Object7.insert("price", QJsonValue::fromVariant(59.95));
	Profiles_S9.append(Profile_S9_Object7);

	QJsonObject Profile_S9_Object8;
	Profile_S9_Object8.insert("name", "normal 13.5th_LPM");
	Profile_S9_Object8.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object8);

	QJsonObject Profile_S9_Object9;
	Profile_S9_Object9.insert("name", "normal 14th_LPM");
	Profile_S9_Object9.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object9);

	QJsonObject Profile_S9_Object10;
	Profile_S9_Object10.insert("name", "normal 14.5th_LPM");
	Profile_S9_Object10.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object10);

	QJsonObject Profile_S9_Object11;
	Profile_S9_Object11.insert("name", "light overclock 15th");
	Profile_S9_Object11.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object11);

	QJsonObject Profile_S9_Object12;
	Profile_S9_Object12.insert("name", "light overclock 15th_LPM");
	Profile_S9_Object12.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object12);

	QJsonObject Profile_S9_Object13;
	Profile_S9_Object13.insert("name", "overclock 16th");
	Profile_S9_Object13.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object13);

	QJsonObject Profile_S9_Object14;
	Profile_S9_Object14.insert("name", "overclock 16th_LPM");
	Profile_S9_Object14.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object14);

	QJsonObject Profile_S9_Object15;
	Profile_S9_Object15.insert("name", "overclock 17th");
	Profile_S9_Object15.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object15);

	QJsonObject Profile_S9_Object16;
	Profile_S9_Object16.insert("name", "overclock 18th");
	Profile_S9_Object16.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object16);
	
	QJsonObject Profile_S9_Object17;
	Profile_S9_Object17.insert("name", "manual");
	Profile_S9_Object17.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_S9.append(Profile_S9_Object17);

	gAppConfig->JSONData.insert("profiles_S9", Profiles_S9);

	profilesCount=gAppConfig->JSONData.value("profiles_S9").toArray().count();
	for(profile=0; profile<profilesCount; profile++)
	{
		QAction *profileAction=new QAction(gAppConfig->JSONData.value("profiles_S9").toArray().at(profile).toObject().value("name").toString());
		ui->menuS9Profiles->addAction(profileAction);
		connect(profileAction, SIGNAL(triggered(bool)), this, SLOT(setOCProfile()));
	}

	gAppConfig->JSONData.remove("profiles_L3");
	QJsonArray Profiles_L3;

	QJsonObject Profile_L3_Object1;
	Profile_L3_Object1.insert("name", "504");
	Profile_L3_Object1.insert("price", QJsonValue::fromVariant(19.95));
	Profiles_L3.append(Profile_L3_Object1);

	QJsonObject Profile_L3_Object2;
	Profile_L3_Object2.insert("name", "580");
	Profile_L3_Object2.insert("price", QJsonValue::fromVariant(29.95));
	Profiles_L3.append(Profile_L3_Object2);

	QJsonObject Profile_L3_Object3;
	Profile_L3_Object3.insert("name", "620");
	Profile_L3_Object3.insert("price", QJsonValue::fromVariant(39.95));
	Profiles_L3.append(Profile_L3_Object3);

	QJsonObject Profile_L3_Object4;
	Profile_L3_Object4.insert("name", "660");
	Profile_L3_Object4.insert("price", QJsonValue::fromVariant(49.95));
	Profiles_L3.append(Profile_L3_Object4);

	QJsonObject Profile_L3_Object5;
	Profile_L3_Object5.insert("name", "680");
	Profile_L3_Object5.insert("price", QJsonValue::fromVariant(59.95));
	Profiles_L3.append(Profile_L3_Object5);

	QJsonObject Profile_L3_Object6;
	Profile_L3_Object6.insert("name", "720");
	Profile_L3_Object6.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_L3.append(Profile_L3_Object6);

	gAppConfig->JSONData.insert("profiles_L3", Profiles_L3);

	profilesCount=gAppConfig->JSONData.value("profiles_L3").toArray().count();
	for(profile=0; profile<profilesCount; profile++)
	{
		QAction *profileAction=new QAction(gAppConfig->JSONData.value("profiles_L3").toArray().at(profile).toObject().value("name").toString());
		ui->menuL3PProfiles->addAction(profileAction);
		connect(profileAction, SIGNAL(triggered(bool)), this, SLOT(setOCProfile()));
	}

	gAppConfig->JSONData.remove("profiles_T9Plus");
	QJsonArray Profiles_T9Plus;

   QJsonObject Profile_T9Plus_Object1;
	Profile_T9Plus_Object1.insert("name", "10.5th");
	Profile_T9Plus_Object1.insert("price", QJsonValue::fromVariant(19.95));
	Profiles_T9Plus.append(Profile_T9Plus_Object1);
	
	QJsonObject Profile_T9Plus_Object2;
	Profile_T9Plus_Object2.insert("name", "10.5th_lpm");
	Profile_T9Plus_Object2.insert("price", QJsonValue::fromVariant(19.95));
	Profiles_T9Plus.append(Profile_T9Plus_Object2);

	QJsonObject Profile_T9Plus_Object3;
	Profile_T9Plus_Object3.insert("name", "11.8th");
	Profile_T9Plus_Object3.insert("price", QJsonValue::fromVariant(39.95));
	Profiles_T9Plus.append(Profile_T9Plus_Object3);

	QJsonObject Profile_T9Plus_Object4;
	Profile_T9Plus_Object4.insert("name", "11.8th_lpm");
	Profile_T9Plus_Object4.insert("price", QJsonValue::fromVariant(49.95));
	Profiles_T9Plus.append(Profile_T9Plus_Object4);

	QJsonObject Profile_T9Plus_Object6;
	Profile_T9Plus_Object6.insert("name", "12.5th");
	Profile_T9Plus_Object6.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_T9Plus.append(Profile_T9Plus_Object6);
	
	QJsonObject Profile_T9Plus_Object7;
	Profile_T9Plus_Object7.insert("name", "12.5th_lpm");
	Profile_T9Plus_Object7.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_T9Plus.append(Profile_T9Plus_Object7);
	
	QJsonObject Profile_T9Plus_Object8;
	Profile_T9Plus_Object8.insert("name", "13.8th");
	Profile_T9Plus_Object8.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_T9Plus.append(Profile_T9Plus_Object8);
	
	QJsonObject Profile_T9Plus_Object9;
	Profile_T9Plus_Object9.insert("name", "13.8th_lpm");
	Profile_T9Plus_Object9.insert("price", QJsonValue::fromVariant(69.95));
	Profiles_T9Plus.append(Profile_T9Plus_Object9);

	gAppConfig->JSONData.insert("profiles_T9Plus", Profiles_T9Plus);

	profilesCount=gAppConfig->JSONData.value("profiles_T9Plus").toArray().count();
	for(profile=0; profile<profilesCount; profile++)
	{
		QAction *profileAction=new QAction(gAppConfig->JSONData.value("profiles_T9Plus").toArray().at(profile).toObject().value("name").toString());
		ui->menuT9PProfiles->addAction(profileAction);
		connect(profileAction, SIGNAL(triggered(bool)), this, SLOT(setOCProfile()));
	}
}

void MainWindow::setOCProfile()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
	QAction *senderAction=qobject_cast<QAction *>(sender());

	QStringList HostsToOC;
	for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
	{
		HostsToOC.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
	}
	if(HostsToOC.isEmpty())
	{
		gLogger->Log("Host list is empty =/", LOG_NOTICE);
		return;
	}
	for(int host=0; host<HostsToOC.count(); host++)
	{
		for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
		{
			if(HostsToOC.at(host)==DefaultTabWidget->DeviceList->at(device)->Address.toString())
			{
				DefaultTabWidget->DeviceList->at(device)->SendCommand(QByteArray("select_profile|")+senderAction->text().toUtf8());
				break;
			}
		}
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
//	sw->QuickAPIScan(DeviceList);
}

void MainWindow::updateDeviceView()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *catw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
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
		item[10]->setData(Qt::DisplayRole, catw->DeviceList->at(row)->CurrentOCProfile);
		item[10]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

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
	if(AddrTo.toIPv4Address()>AddrFrom.toIPv4Address())
	{
		for(quint32 address=AddrFrom.toIPv4Address(); address<=AddrTo.toIPv4Address(); address++)
		{
			ASICDevice *newDevice;
			newDevice=new(ASICDevice);
			newDevice->Address=QHostAddress(address);
			DefaultTabWidget->addDevice(newDevice);
		}
		return;
	}
	ASICDevice *NewDevice;
	NewDevice=new(ASICDevice);
	NewDevice->Address=AddrFrom;
	DefaultTabWidget->addDevice(NewDevice);
	updateDeviceView();
}

void MainWindow::addNewGroup(QString title, QString description, QString username, QString password, quint16 apiport, quint16 webport)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *qweWidget=new ASICTableWidget(this);
	qweWidget->setSortingEnabled(true);
	qweWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	qweWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	qweWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	qweWidget->setWordWrap(false);
	qweWidget->setShowGrid(false);
	qweWidget->setIconSize(QSize(32, 32));

	qweWidget->setColumnCount(ColumnTitles->count());
	qweWidget->setHorizontalHeaderLabels(*ColumnTitles);

	qweWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::QHeaderView::Interactive);
	//qweWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	//qweWidget->horizontalHeader()->setSectionResizeMode(qweWidget->horizontalHeader()->count()-1, QHeaderView::Stretch);
	qweWidget->verticalHeader()->setHidden(true);

	qweWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(qweWidget, SIGNAL(customContextMenuRequested(QPoint)),  this, SLOT(on_customContextMenuRequested(QPoint)));

	qweWidget->Title=title;
	qweWidget->Description=description;
	qweWidget->UserName=username;
	qweWidget->Password=password;
	qweWidget->APIPort=apiport;
	qweWidget->WebPort=webport;
	qweWidget->GroupID=GroupsCount;

	ui->tabWidget->addTab(qweWidget, qweWidget->Title);

	if(0==GroupsCount)
	{
		delete(qweWidget->DeviceList);
		qweWidget->DeviceList=DeviceList;
		DefaultTabWidget=qweWidget;
	}
	QAction *qweAction=new QAction(title, qweWidget);
	ui->menuMove_devices_to->addAction(qweAction);
	connect(qweAction, SIGNAL(triggered(bool)), this, SLOT(addDevicesToGroup()));
	GroupsCount++;
}

void MainWindow::on_rebootButton_clicked()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
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
		for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
		{
			if(HostsToReboot.at(host)==DefaultTabWidget->DeviceList->at(device)->Address.toString())
			{
				PickedDevice=DefaultTabWidget->DeviceList->at(device);
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
	DefaultTabWidget->addDevice(device);
}

void MainWindow::addDevicesToGroup()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());

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
		for(int i=0; i<DefaultTabWidget->DeviceList->count(); i++)
		{
			if(HostsToAdd.at(host)==DefaultTabWidget->DeviceList->at(i)->Address.toString())
			{
				senderParentWidget->addDevice(DefaultTabWidget->DeviceList->at(i));
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

	ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
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
		for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
		{
			if(HostsToFlash.at(host)==DefaultTabWidget->DeviceList->at(device)->Address.toString())
			{
				uploadFirmware(DefaultTabWidget->DeviceList->at(device));
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
	for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
	{
		if(reply->request().url().host()==DefaultTabWidget->DeviceList->at(device)->Address.toString())
		{
			gLogger->Log("now reboot device "+reply->request().url().host().toStdString(), LOG_INFO);
			DefaultTabWidget->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/reboot.cgi");
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
	ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
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
		for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
		{
			if(HostsToReset.at(host)==DefaultTabWidget->DeviceList->at(device)->Address.toString())
			{
				gLogger->Log("now reset device "+HostsToReset.at(host).toStdString(), LOG_INFO);
				DefaultTabWidget->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/reset_conf.cgi");
				break;
			}
		}
	}
}

void MainWindow::on_actionToggle_fullscreen_triggered()
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	if(mw->isFullScreen())
	{
		mw->showNormal();
	}
	else
	{
		mw->showFullScreen();
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
	ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
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
		for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
		{
			if(HostsToRemove.at(host)==DefaultTabWidget->DeviceList->at(device)->Address.toString())
			{
				gLogger->Log("now remove the device"+HostsToRemove.at(host).toStdString(), LOG_INFO);
				ASICDevice *deviceToRemove=DefaultTabWidget->DeviceList->at(device);
				if(ctw==DefaultTabWidget)
				{
					for(int tab=0; tab<ui->tabWidget->count(); tab++)
					{
						ASICTableWidget *tw=qobject_cast<ASICTableWidget *>(ui->tabWidget->widget(tab));
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
	ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
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
	for(int host=0; host<HostsToSetup.count(); host++)
	{
		PickedDevice=nullptr;
		DataToSend.clear();
		if(SettingsToSendJSON.value("pools").isArray())
		{
			SettingsToSendJSON.remove("pools");
		}
		for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
		{
			if(HostsToSetup.at(host)==DefaultTabWidget->DeviceList->at(device)->Address.toString())
			{
				PickedDevice=DefaultTabWidget->DeviceList->at(device);
				break;
			}
		}
		if(!PickedDevice)
		{
			gLogger->Log("cannot find the device with address "+HostsToSetup.at(host).toStdString(), LOG_INFO);
			continue;
		}

		for(setting=0; setting<settings.count(); setting++)
		{
			if(gAppConfig->AddressBasedPostfixToWorkerName)
			{
				if(settings.at(setting).split('=').first()==QString("pool1user"))
				{
					if(2==settings.at(setting).split('=', QString::SkipEmptyParts).count())
					{
						DataToSend.append(QString("_ant_pool1user")+
										  QString("=")+
										  settings.at(setting).split('=').last()+
										  QString(".")+
										  HostsToSetup.at(host).split('.').at(2)+
										  QString("x")+
										  HostsToSetup.at(host).split('.').at(3)+
										  QString(" "));
					}
					else
					{
						DataToSend.append(QString("_ant_pool1user")+
										  QString("= "));
					}
				}
				else if(settings.at(setting).split('=').first()==QString("pool2user"))
				{
					if(2==settings.at(setting).split('=', QString::SkipEmptyParts).count())
					{
						DataToSend.append(QString("_ant_pool2user")+
										  QString("=")+
										  settings.at(setting).split('=').last()+
										  QString(".")+
										  HostsToSetup.at(host).split('.').at(2)+
										  QString("x")+
										  HostsToSetup.at(host).split('.').at(3)+
										  QString(" "));
					}
					else
					{
						DataToSend.append(QString("_ant_pool2user")+
										  QString("= "));
					}
				}
				else if(settings.at(setting).split('=').first()==QString("pool3user"))
				{
					if(2==settings.at(setting).split('=', QString::SkipEmptyParts).count())
					{
						DataToSend.append(QString("_ant_pool3user")+
										  QString("=")+
										  settings.at(setting).split('=').last()+
										  QString(".")+
										  HostsToSetup.at(host).split('.').at(2)+
										  QString("x")+
										  HostsToSetup.at(host).split('.').at(3)+
										  QString(" "));
					}
					else
					{
						DataToSend.append(QString("_ant_pool3user")+
										  QString("= "));
					}
				}
				else
				{
					DataToSend.append(QString("_ant_")+settings.at(setting)+QString(" "));
				}
			}
			else
			{
				DataToSend.append(QString("_ant_")+settings.at(setting)+QString(" "));
			}
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
	ui->tabWidget->removeTab(index);
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
	gLogger->Log("MainWindow::"+string(__FUNCTION__), LOG_DEBUG);
	ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
	gLogger->Log("tab index '"+to_string(index)+"'", LOG_DEBUG);
	gLogger->Log("tab Title '"+ctw->Title.toStdString()+"'", LOG_DEBUG);
	gLogger->Log("tab Description '"+ctw->Description.toStdString()+"'", LOG_DEBUG);
	gLogger->Log("tab UserName '"+ctw->UserName.toStdString()+"'", LOG_DEBUG);
	gLogger->Log("tab Password '"+ctw->Password.toStdString()+"'", LOG_DEBUG);
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
	emit(NeedToShowAddNewGroupDialog());
}

void MainWindow::on_deviceSettingsButton_clicked()
{
	emit(NeedToShowDeviceSettingsWindow());
}

void MainWindow::on_searchButton_clicked()
{
	emit(NeedToShowScannerWindow());
}

