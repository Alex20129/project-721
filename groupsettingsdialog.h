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
	void newGroupCreated(ASICTableWidget *new_group_widget);
	void groupSettingsUpdated(ASICTableWidget *group_widget);
public:
    explicit GroupSettingsDialog(QWidget *parent=nullptr);
    ~GroupSettingsDialog();
public slots:
	void showGroupSettings(ASICTableWidget *group_widget);
private slots:
    void on_buttonBox_accepted();
	void on_buttonBox_rejected();

private:
	ASICTableWidget *pGroupWidget;
    Ui::GroupSettingsDialog *ui;
};

#endif // GROUPSETTINGSDIALOG_H
