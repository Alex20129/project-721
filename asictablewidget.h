#ifndef ASICTABLEWIDGET_H
#define ASICTABLEWIDGET_H

#include <QTableWidget>
#include <QDesktopServices>
#include "asicdevice.h"

class QTableWidgetIPItem : public QTableWidgetItem
{
public:
    virtual QVariant data(int role) const
    {
        if(role==Qt::DisplayRole)
        {
            QHostAddress addr(QTableWidgetItem::data(role).toUInt());
            return QVariant(addr.toString());
        }
        return QTableWidgetItem::data(role);
    }
};

class ASICTableWidget : public QTableWidget
{
	Q_OBJECT
signals:
	void GroupSettingsHaveBeenChanged();
public slots:
	void addDevice(ASICDevice *deviceToAdd);
    void removeDevice(ASICDevice *deviceToRemove);
    void on_cellDoubleClicked(int row, int column);
public:
    explicit ASICTableWidget(QWidget *parent=nullptr);
    QVector <ASICDevice *> *DeviceList;
	quint16 WebPort();
	quint16 APIPort();
	uint GroupID();
	QString Title();
	QString Description();
	QString UserName();
	QString Password();
	void SetWebPort(quint16 port);
	void SetAPIPort(quint16 port);
	void SetGroupID(uint group_id);
	void SetTitle(QString title);
	void SetDescription(QString description);
	void SetUserName(QString user_name);
	void SetPassword(QString password);
private slots:
	void ApplyGroupSettingsToAllDevices();
private:
	quint16 pWebPort;
	quint16 pAPIPort;
	uint pGroupID;
	QString pTitle;
	QString pDescription;
	QString pUserName;
	QString pPassword;
};

#endif // ASICTABLEWIDGET_H
