#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QtNetwork>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QCloseEvent>
#include <QDesktopServices>
#include "asictablewidget.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
signals:
    void FirmwareUploadProgess(int progress);
    void timeToSleep();
    void timeToWakeUp();
	void NeedToRescanDevices(QVector<ASICDevice *> *devices);
	void NeedToShowBasicSettingsWindow();
	void NeedToShowNetworkSettingsWindow();
	void NeedToShowDeviceSettingsWindow();
	void NeedToShowScannerWindow();
	void NeedToShowSleepSettingsWindow();
	void NeedToShowAddNewDeviceDialog();
	void NeedToShowAddNewGroupDialog();
public slots:
    void updateDeviceView();
    void on_customContextMenuRequested(QPoint position);
	void addDevice(ASICDevice *device);
    void addDevicesToGroup();
    void addNewDevices(QString addressFrom, QString addressTo);
    void addNewGroup(QString title, QString description, QString username, QString password, quint16 apiport, quint16 webport);
    void uploadSettings(QStringList settings);
public:
    explicit MainWindow(QWidget *parent=nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);
    void uploadFirmware(ASICDevice *device);
    bool IsAwake;
    QTimer *RefreshTimer, *SleepWakeTimer;
    QStringList *ColumnTitles;

private slots:
    void rescanDevices();
    void sleepOrWake();
    void on_timeToSleep();
    void on_timeToWakeUp();
    void on_updateButton_clicked();
    void on_rebootButton_clicked();
    void authenticationHandler(QNetworkReply *repl, QAuthenticator *auth);
    void uploadFirmwareFinished(QNetworkReply *reply);
    void on_firmwareButton_clicked();
    void on_actionToggle_fullscreen_triggered();
    void on_actionGroup_summary_triggered();
    void on_actionRemove_devices_from_group_triggered();
    void on_actionSupport_website_triggered();
    void on_actionReset_to_default_triggered();
	void on_tabWidget_tabCloseRequested(int index);
	void on_tabWidget_tabBarDoubleClicked(int index);
	void on_actionBasic_settings_triggered();
	void on_actionNetwork_settings_triggered();
	void on_actionDevice_settings_triggered();
	void on_actionFind_devices_triggered();
	void on_actionSleep_settings_triggered();
	void on_actionAdd_devices_triggered();
	void on_actionAdd_group_triggered();
	void on_deviceSettingsButton_clicked();
	void on_searchButton_clicked();

private:
	ASICTableWidget *DefaultTabWidget;
	QVector <ASICDevice *> *DefaultDeviceList;
    int loadTabs();
    void saveTabs();
    uint ActiveUploadingThreads;
    QByteArray *firmwareData;
    uint GroupsCount;
    Ui::MainWindow *ui;
};

extern MainWindow *MainWin;

#endif // MAINWINDOW_HPP
