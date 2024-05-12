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
public slots:
	void addDevice(ASICDevice *deviceToAdd);
    void removeDevice(ASICDevice *deviceToRemove);
    void on_cellDoubleClicked(int row, int column);
public:
    explicit ASICTableWidget(QWidget *parent=nullptr);
    QString UserName, Password, Description, Title;
    quint16 WebPort;
    quint16 APIPort;
    uint GroupID;
    QVector <ASICDevice *> *DeviceList;
};

#endif // ASICTABLEWIDGET_H
