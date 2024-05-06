#include "mainwindow.h"
#include "logger.hpp"
#include "ui_mainwindow.h"
#include "main.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ActiveUploadingThreads=0;
    GroupsCount=0;

    firmwareData=new(QByteArray);

    ColumnTitles=new QStringList({"Address", "Type", "Miner", "HashRate", "Temperature", "Frequency", "Uptime", "Hardware Errors", "Pool", "User", "OC Profile"});

    RefreshTimer=new(QTimer);
    RefreshTimer->setInterval(DEFAULT_UPDATE_INTERVAL);
    connect(RefreshTimer, SIGNAL(timeout()), this, SLOT(rescanDevices()));
    connect(RefreshTimer, SIGNAL(timeout()), this, SLOT(updateDeviceView()));

    SleepWakeTimer=new(QTimer);
    SleepWakeTimer->setInterval(2000);
    connect(SleepWakeTimer, SIGNAL(timeout()), this, SLOT(sleepOrWake()));

    IsAwake=true;

    ui->setupUi(this);
    connect(ui->actionBasic_settings, SIGNAL(triggered(bool)), bsw, SLOT(show()));
    connect(ui->actionNetwork_settings, SIGNAL(triggered(bool)), nsw, SLOT(show()));
    connect(ui->actionSleep_settings, SIGNAL(triggered(bool)), ssw, SLOT(show()));
    connect(ui->actionDevice_settings, SIGNAL(triggered(bool)), devsw, SLOT(show()));
    connect(ui->actionFind_devices, SIGNAL(triggered(bool)), sw, SLOT(show()));
    connect(ui->actionAdd_devices, SIGNAL(triggered(bool)), andd, SLOT(show()));
    connect(ui->actionAdd_group, SIGNAL(triggered(bool)), angd, SLOT(show()));

    connect(ui->deviceSettingsButton, SIGNAL(clicked(bool)), devsw, SLOT(show()));
    connect(ui->searchButton, SIGNAL(clicked(bool)), sw, SLOT(show()));

    connect(sw, SIGNAL(ScanIsDone()), this, SLOT(updateDeviceView()));

    connect(this, SIGNAL(FirmwareUploadProgess(int)), ui->progressBar, SLOT(setValue(int)));

    connect(angd, SIGNAL(groupDataObtained(QString, QString, QString, QString, quint16, quint16)), this, SLOT(addNewGroup(QString, QString, QString, QString, quint16, quint16)));
    connect(andd, SIGNAL(deviceDataObtained(QString, QString)), this, SLOT(addNewDevices(QString, QString)));
    connect(devsw, SIGNAL(deviceSettingsObtained(QStringList)), this, SLOT(uploadSettings(QStringList)));

    connect(this, SIGNAL(timeToSleep()), this, SLOT(on_timeToSleep()));
    connect(this, SIGNAL(timeToWakeUp()), this, SLOT(on_timeToWakeUp()));

    this->setWindowTitle(QString(PROGRAM_NAME)+QString(" ")+QString(PROGRAM_VERSION));

    if(!loadTabs())
    {
        angd->show();
    }
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateDeviceView()));
    loadProfiles();
}

MainWindow::~MainWindow()
{
    delete(RefreshTimer);
    delete ui;
}

void MainWindow::sleepOrWake()
{
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
    for(device=0; device<GlobalDeviceList.count(); device++)
    {
        QJsonObject DeviceObject;
        DeviceObject.insert("address", GlobalDeviceList.at(device)->Address.toString());
        DeviceObject.insert("description", GlobalDeviceList.at(device)->Description);
        DeviceObject.insert("group", QJsonValue::fromVariant(GlobalDeviceList.at(device)->GroupID));
        Devices.append(DeviceObject);
        gAppConfig->JSONData.insert("devices", Devices);
    }
}

void MainWindow::loadProfiles()
{
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
    ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
    QAction *senderAction=qobject_cast<QAction *>(sender());

    QStringList HostsToOC;
    for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
    {
        HostsToOC.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
    }
    if(HostsToOC.isEmpty())
    {
        gAppLogger->Log("Host list is empty =/");
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
    if(event->isAccepted())
    {
        saveTabs();
        QApplication::exit(0);
    }
}

void MainWindow::rescanDevices()
{
    sw->QuickAPIScan(GlobalDeviceList);
}

void MainWindow::updateDeviceView()
{
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
        qweWidget->DeviceList=&GlobalDeviceList;
        DefaultTabWidget=qweWidget;
    }
    QAction *qweAction=new QAction(title, qweWidget);
    ui->menuMove_devices_to->addAction(qweAction);
    connect(qweAction, SIGNAL(triggered(bool)), this, SLOT(addDevicesToGroup()));
    GroupsCount++;
}

void MainWindow::on_rebootButton_clicked()
{
    ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
    QStringList HostsToReboot;
    ASICDevice *PickedDevice;
    for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
    {
        HostsToReboot.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
    }
    if(HostsToReboot.isEmpty())
    {
        gAppLogger->Log("Host list is empty =/");
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
            gAppLogger->Log("cannot find the device with address");
            gAppLogger->Log(HostsToReboot.at(host));
            gAppLogger->Log("continue...");
            continue;
        }
        gAppLogger->Log("now reboot device");
        gAppLogger->Log(PickedDevice->Address.toString());

		PickedDevice->ExecCGIScriptViaGET("/cgi-bin/reboot.cgi");
    }
}

void MainWindow::addDevicesToGroup()
{
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
        gAppLogger->Log("Host list is empty =/");
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
    Q_UNUSED(position)
    ui->menuControl->exec(QCursor::pos());
}

void MainWindow::on_firmwareButton_clicked()
{
    QFile *firmwareFile;
    firmwareFile=new QFile(QFileDialog::getOpenFileName(this, "Choose firmware...", "", "Tar-GZip archive (*.tar.gz);; Tar-BZip2 archive (*.tar.bz2)"));
    if(firmwareFile->fileName().isEmpty())
    {
        gAppLogger->Log("Filename is empty =/");
        return;
    }
    if(!firmwareFile->open(QIODevice::ReadOnly))
    {
        gAppLogger->Log("Cannot open the file =\\");
        gAppLogger->Log(firmwareFile->errorString());
        return;
    }
    *firmwareData=firmwareFile->readAll();

    firmwareFile->close();
    gAppLogger->Log("Firmware has been loaded from");
    gAppLogger->Log(firmwareFile->fileName());
    delete(firmwareFile);

    ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
    QStringList HostsToFlash;
    for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
    {
        HostsToFlash.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
    }
    if(HostsToFlash.isEmpty())
    {
        gAppLogger->Log("Host list is empty =/");
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
    QUrl deviceURL;
    deviceURL.setScheme("http");
    deviceURL.setHost(device->Address.toString());
    deviceURL.setPort(device->WebPort);
    deviceURL.setUserName(device->UserName);
    deviceURL.setPassword(device->Password);
    if(gAppConfig->ClearUpSettingsWhenFirmwareUpdate)
    {
        deviceURL.setPath("/cgi-bin/upgrade_clear.cgi");
        qInfo()<<"[upgrade_clear.cgi]";
    }
    else
    {
        deviceURL.setPath("/cgi-bin/upgrade.cgi");
        qInfo()<<"[upgrade.cgi]";
    }
    qInfo()<<"now upload firmware on"<<deviceURL.toString();
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
    qInfo()<<"authenticationHandler";
    qInfo()<<repl->url();
    qInfo()<<auth->user();
    qInfo()<<auth->password();
}

void MainWindow::uploadFirmwareFinished(QNetworkReply *reply)
{
    ActiveUploadingThreads--;
    QByteArray ReceivedData;
    if(reply->isReadable())
    {
        ReceivedData=reply->readAll();
    }
    if(reply->error()==QNetworkReply::NoError)
    {
        gAppLogger->Log("MainWindow::uploadFirmwareFinished reply success");
    }
    else
    {
        gAppLogger->Log("MainWindow::uploadFirmwareFinished reply error");
        gAppLogger->Log(reply->errorString());
    }
    reply->manager()->disconnect();
    reply->manager()->deleteLater();
    gAppLogger->Log(QString(ReceivedData));
    for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
    {
        if(reply->request().url().host()==DefaultTabWidget->DeviceList->at(device)->Address.toString())
        {
            gAppLogger->Log("now reboot device");
            gAppLogger->Log(reply->request().url().host());
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
    ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
    QStringList HostsToReset;
    for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
    {
        HostsToReset.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
    }
    if(HostsToReset.isEmpty())
    {
        gAppLogger->Log("Host list is empty =/");
        return;
    }
    for(int host=0; host<HostsToReset.count(); host++)
    {
        for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
        {
            if(HostsToReset.at(host)==DefaultTabWidget->DeviceList->at(device)->Address.toString())
            {
                gAppLogger->Log("now reset the device");
                gAppLogger->Log(HostsToReset.at(host));
                DefaultTabWidget->DeviceList->at(device)->ExecCGIScriptViaGET("/cgi-bin/reset_conf.cgi");
                break;
            }
        }
    }
}

void MainWindow::on_actionToggle_fullscreen_triggered()
{
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
    ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
    QStringList HostsToRemove;
    for(int i=0; i<ctw->selectionModel()->selectedRows().size(); i++)
    {
        HostsToRemove.append(ctw->selectionModel()->selectedRows().at(i).data().toString());
    }
    if(HostsToRemove.isEmpty())
    {
        gAppLogger->Log("Host list is empty =/");
        return;
    }
    for(int host=0; host<HostsToRemove.count(); host++)
    {
        for(int device=0; device<DefaultTabWidget->DeviceList->count(); device++)
        {
            if(HostsToRemove.at(host)==DefaultTabWidget->DeviceList->at(device)->Address.toString())
            {
                gAppLogger->Log("now remove the device");
                gAppLogger->Log(HostsToRemove.at(host));
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
        gAppLogger->Log("Host list is empty =/");
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
            gAppLogger->Log("cannot find the device with address");
            gAppLogger->Log(HostsToSetup.at(host));
            gAppLogger->Log("continue...");
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

		gAppLogger->Log(PickedDevice->Address.toString()+" will be configured now");
		PickedDevice->UploadDataViaPOST("/cgi-bin/set_miner_conf.cgi", &DataToSend);
		PickedDevice->ExecCGIScriptViaGET("/cgi-bin/reboot.cgi");
    }
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    ui->tabWidget->widget(index)->deleteLater();
    ui->tabWidget->removeTab(index);
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
    ASICTableWidget *ctw=qobject_cast<ASICTableWidget *>(ui->tabWidget->currentWidget());
    gAppLogger->Log("tab was double-clicked");
    gAppLogger->Log(QString("tab index '")+QString::number(index)+QString("'"));
    gAppLogger->Log(QString("tab Title '")+ctw->Title+QString("'"));
    gAppLogger->Log(QString("tab Description '")+ctw->Description+QString("'"));
    gAppLogger->Log(QString("tab UserName '")+ctw->UserName+QString("'"));
    gAppLogger->Log(QString("tab Password '")+ctw->Password+QString("'"));
}

void MainWindow::on_actionSupport_website_triggered()
{
	QUrl SupportOnSite("https://www.google.com");
    QDesktopServices::openUrl(SupportOnSite);
}
