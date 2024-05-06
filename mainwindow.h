#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
public slots:
    void updateDeviceView();
    void on_customContextMenuRequested(QPoint position);
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
    ASICTableWidget *DefaultTabWidget;
private slots:
    void setOCProfile();
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
    void on_tabWidget_tabCloseRequested(int index);
    void on_tabWidget_tabBarDoubleClicked(int index);
    void on_actionSupport_website_triggered();
    void on_actionReset_to_default_triggered();

private:
    int loadTabs();
    void saveTabs();
    void loadProfiles();
    uint ActiveUploadingThreads;
    QByteArray *firmwareData;
    uint GroupsCount;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
