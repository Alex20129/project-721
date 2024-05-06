#ifndef ADDNEWDEVICEDIALOG_H
#define ADDNEWDEVICEDIALOG_H

#include <QDialog>
#include "asicdevice.h"

namespace Ui {
class AddNewDeviceDialog;
}

class AddNewDeviceDialog : public QDialog
{
    Q_OBJECT
signals:
    void deviceDataObtained(QString addressFrom, QString addressTo);
public:
    explicit AddNewDeviceDialog(QWidget *parent=nullptr);
    ~AddNewDeviceDialog();

private slots:
    void on_AddNewDeviceDialog_accepted();

private:
    Ui::AddNewDeviceDialog *ui;
};

#endif // ADDNEWDEVICEDIALOG_H
