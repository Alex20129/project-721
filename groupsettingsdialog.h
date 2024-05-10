#ifndef GROUPSETTINGSDIALOG_H
#define GROUPSETTINGSDIALOG_H

#include <QDialog>

namespace Ui
{
    class GroupSettingsDialog;
}

class GroupSettingsDialog : public QDialog
{
    Q_OBJECT
signals:
    void groupDataObtained(QString title, QString description, QString username, QString password, quint16 apiport, quint16 webport);
public:
    explicit GroupSettingsDialog(QWidget *parent=nullptr);
    ~GroupSettingsDialog();
public slots:
	void showGroupSettings(int group_id);
private slots:
    void on_buttonBox_accepted();

private:
	int pGroupID;
    Ui::GroupSettingsDialog *ui;
};

#endif // GROUPSETTINGSDIALOG_H
