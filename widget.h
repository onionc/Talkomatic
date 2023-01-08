#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtMqtt/qmqttclient.h>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <json11/json11.hpp>
#include <string>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    QString name; // 名称

private slots:
    void on_conBtn_clicked(bool checked);

private:
    Ui::Widget *ui;
    QMqttClient *mClient;

};

#endif // WIDGET_H
