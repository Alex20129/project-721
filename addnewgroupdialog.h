#ifndef ADDNEWGROUPDIALOG_H
#define ADDNEWGROUPDIALOG_H

#include <QDialog>

namespace Ui {
class AddNewGroupDialog;
}

class AddNewGroupDialog : public QDialog
{
    Q_OBJECT
signals:
    void groupDataObtained(QString title, QString description, QString username, QString password, quint16 apiport, quint16 webport);
public:
    explicit AddNewGroupDialog(QWidget *parent=nullptr);
    ~AddNewGroupDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::AddNewGroupDialog *ui;
};

#endif // ADDNEWGROUPDIALOG_H
