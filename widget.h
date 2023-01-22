#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtMqtt/qmqttclient.h>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <json11/json11.hpp>
#include <string>
#include <redis/RedisConnect.h>


#define ENV_PATH "config/local.env"
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
    QString currentRoomName; // 当前房间名称
    shared_ptr<RedisConnect> redis;

private slots:
    void on_conBtn_clicked(bool checked);
    void on_roomBtn_clicked(bool checked);

private:
    Ui::Widget *ui;
    QMqttClient *mClient;
    QMqttClient::ClientState state;


    // 登入和退出
    void login();
    void logout();
    // room
    std::vector<std::string> getRooms(); // 获取房间列表
    bool joinRoom(QString roomName); // 加入房间
    bool leaveRoom(); // 退出房间

    bool initTalk(); // 聊天初始化


};

#endif // WIDGET_H
