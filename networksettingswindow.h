#ifndef NETWORKSETTINGSWINDOW_H
#define NETWORKSETTINGSWINDOW_H

#include <QWidget>
#include <QKeyEvent>

namespace Ui {
class NetworkSettingsWindow;
}

class NetworkSettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkSettingsWindow(QWidget *parent=nullptr);
    ~NetworkSettingsWindow();
    void keyPressEvent(QKeyEvent *event);
private slots:
    void on_applyButton_clicked();

private:
    Ui::NetworkSettingsWindow *ui;
};

#endif // NETWORKSETTINGSWINDOW_H
