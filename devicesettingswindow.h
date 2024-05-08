#ifndef DEVICESETTINGSWINDOW_H
#define DEVICESETTINGSWINDOW_H

#include <QWidget>
#include <QJsonObject>
#include <QJsonDocument>
#include <QKeyEvent>

namespace Ui
{
	class DeviceSettingsWindow;
}

class DeviceSettingsWindow : public QWidget
{
    Q_OBJECT
signals:
    void deviceSettingsObtained(QStringList settings);
public:
    explicit DeviceSettingsWindow(QWidget *parent=nullptr);
    ~DeviceSettingsWindow();
    void keyPressEvent(QKeyEvent *event);
private slots:
    void on_applyButton_clicked();
private:
    Ui::DeviceSettingsWindow *ui;
};

#endif // DEVICESETTINGSWINDOW_H
