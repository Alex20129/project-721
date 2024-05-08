#ifndef SCANNERWINDOW_H
#define SCANNERWINDOW_H

#include <QWidget>
#include <QtNetwork>
#include <QListWidget>
#include <QKeyEvent>
#include "asicdevice.h"

namespace Ui
{
	class ScannerWindow;
}

class ScannerWindow : public QWidget
{
	Q_OBJECT
signals:
	void ScanIsRun();
	void ScanProgress(int progress);
	void ScanIsDone();
	void DeviceFound(ASICDevice *device);
public:
	explicit ScannerWindow(QWidget *parent=nullptr);
	~ScannerWindow();
	void keyPressEvent(QKeyEvent *event);
	void QuickAPIScan(QVector<ASICDevice *> *devicesToCheck);
public slots:
	void updateDeviceList(ASICDevice *device);
	void clearUpDeviceList(ASICDevice *device);
private:
	Ui::ScannerWindow *ui;
	QVector <ASICDevice *> *pHostsToScan;
	bool pIsBusy, pStopScan;
private slots:
	void on_scanIsDone();
	void on_scanIsRun();
	void on_apiScanButton_clicked();
	void on_knownIPcomboBox_currentIndexChanged(int index);
	void on_stopScanButton_clicked();
};

#endif // SCANNERWINDOW_H
