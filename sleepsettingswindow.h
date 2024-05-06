#ifndef SLEEPSETTINGSWINDOW_H
#define SLEEPSETTINGSWINDOW_H

#include <QWidget>

namespace Ui {
class SleepSettingsWindow;
}

class SleepSettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SleepSettingsWindow(QWidget *parent=nullptr);
    ~SleepSettingsWindow();
    void keyPressEvent(QKeyEvent *event);
private slots:
    void on_applyButton_clicked();

private:
    Ui::SleepSettingsWindow *ui;
};

#endif // SLEEPSETTINGSWINDOW_H
