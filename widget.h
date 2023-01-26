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
#include <QTimer>
#include <QTextCursor>
#include <QScrollBar>

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

    QString name; // 本人名称
    QString dialogistName; // 对方名称
    QString currentRoomName; // 当前房间名称
    shared_ptr<RedisConnect> redis;


private slots:
    void on_conBtn_clicked(bool checked);
    void on_roomBtn_clicked(bool checked);

    void slot_getInputMsg(); // 获取输入消息
    void refreshRoomList(); // 刷新房间列表
    bool getRoomState(); // 获取房间状态，true代表有其他的一人或没有，false代表错误

private:
    Ui::Widget *ui;
    QMqttClient *mClient;
    QMqttClient::ClientState state;


    // 登入和退出相关操作

    void logout();
    // room
    std::vector<std::string> getRooms(); // 获取房间列表
    bool joinRoom(QString roomName); // 加入房间
    bool leaveRoom(); // 退出房间

    bool initTalk(); // 聊天初始化

    // mqtt消息
    bool sendMsg(QString topic, QString username, QString msg, int scrollBarPos=0); // 发送mqtt消息

    // 定时器
    QTimer *refreshRoomTimer; // 刷新房间列表
    QTimer *refreshRoomStateTimer; // 刷新房间状态


};

#endif // WIDGET_H
