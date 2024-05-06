#ifndef BASICSETTINGSWINDOW_H
#define BASICSETTINGSWINDOW_H

#include <QWidget>

namespace Ui {
class BasicSettingsWindow;
}

class BasicSettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit BasicSettingsWindow(QWidget *parent=nullptr);
    ~BasicSettingsWindow();
    void keyPressEvent(QKeyEvent *event);
private slots:
    void on_applyButton_clicked();

private:
    Ui::BasicSettingsWindow *ui;
};

#endif // BASICSETTINGSWINDOW_H
