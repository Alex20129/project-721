#ifndef GROUPSETTINGSDIALOG_H
#define GROUPSETTINGSDIALOG_H

#include <QDialog>
#include "asictablewidget.h"

namespace Ui
{
    class GroupSettingsDialog;
}

class GroupSettingsDialog : public QDialog
{
    Q_OBJECT
signals:
	void newGroupCreated(ASICTableWidget *new_group);
	void groupSettingsUpdated();
public:
    explicit GroupSettingsDialog(QWidget *parent=nullptr);
    ~GroupSettingsDialog();
public slots:
	void showGroupSettings(int group_id);
private slots:
    void on_buttonBox_accepted();
	void on_buttonBox_rejected();

private:
	int pGroupID;
    Ui::GroupSettingsDialog *ui;
};

#endif // GROUPSETTINGSDIALOG_H
