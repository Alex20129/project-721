#include "addnewgroupdialog.h"
#include "ui_addnewgroupdialog.h"

AddNewGroupDialog::AddNewGroupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddNewGroupDialog)
{
    ui->setupUi(this);
}

AddNewGroupDialog::~AddNewGroupDialog()
{
    delete ui;
}

void AddNewGroupDialog::on_buttonBox_accepted()
{
    quint16 apiPor=static_cast<quint16>(ui->apiport->text().toUInt());
    quint16 webPor=static_cast<quint16>(ui->webport->text().toUInt());
    emit(groupDataObtained(ui->title->text(), ui->description->toPlainText(), ui->username->text(), ui->password->text(), apiPor, webPor));
}
