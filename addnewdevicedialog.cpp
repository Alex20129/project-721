#include "addnewdevicedialog.h"
#include "ui_addnewdevicedialog.h"
#include "asicdevice.h"
#include "main.hpp"

AddNewDeviceDialog::AddNewDeviceDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AddNewDeviceDialog)
{
    ui->setupUi(this);
}

AddNewDeviceDialog::~AddNewDeviceDialog()
{
    delete ui;
}

void AddNewDeviceDialog::on_AddNewDeviceDialog_accepted()
{
    emit(deviceDataObtained(ui->ipAddressFrom->text(), ui->ipAddressTo->text()));
}
