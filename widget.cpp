#include "widget.h"
#include "ui_widget.h"
#include "util.h"

int RedisConnect::POOL_MAXLEN = 8;
int RedisConnect::SOCKET_TIMEOUT = 10;


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 默认值
    ui->RoomGroupBox->hide(); // 默认不显示聊天界面
    currentRoomName = "";
    name = dialogistName = "";

    // mqtt
    mClient = new QMqttClient(this);
    mClient->setHostname(util::readIni(ENV_PATH, "mqtt/hostname").toString()); // todo: 参数放配置文件
    mClient->setPort(util::readIni(ENV_PATH, "mqtt/port").toInt());
    if(!util::readIni(ENV_PATH, "mqtt/username").toString().isEmpty()){
         mClient->setUsername(util::readIni(ENV_PATH, "mqtt/username").toString());
         mClient->setPassword(util::readIni(ENV_PATH, "mqtt/password").toString());
    }
   qDebug()<<util::readIni(ENV_PATH, "mqtt/hostname").toString();

    state = QMqttClient::Disconnected;
    /*
    // 使用定时器刷新房间
    refreshRoomTimer = new QTimer(this);
    refreshRoomTimer->setInterval(1000);
    connect(refreshRoomTimer, &QTimer::timeout, this, &Widget::refreshRoomList);
    */


    // 使用定时器刷新房间状态
    refreshRoomStateTimer = new QTimer(this);
    refreshRoomStateTimer->setInterval(1000);
    connect(refreshRoomStateTimer, &QTimer::timeout, this, &Widget::getRoomState);

    // 判断连接
    connect(mClient, &QMqttClient::stateChanged, this, [this](){
        state = mClient->state();
        qDebug()<<"state:"<<state;
        if(state == QMqttClient::Connected){
            // 已连接
            this->ui->conBtn->setChecked(true);
            this->ui->nameEdit->setEnabled(false);
            this->ui->conBtn->setText("断开");
        }else if(state == QMqttClient::Disconnected){
            // 断开连接
            this->ui->conBtn->setChecked(false);
            this->ui->nameEdit->setEnabled(true);
            this->ui->conBtn->setText("连接");
            return;
        }else{
            // 正在连接/其他，继续等待
            return;
        }

        //this->refreshRoomTimer->start();
        refreshRoomList();
    });


    // 接收消息
    connect(mClient, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(" Received Topic: ")
                    + topic.name()
                    + QLatin1String(" Message: ")
                    + message
                    + QLatin1Char('\n');
        qDebug()<<"recv"<<content;

        // 解析 json
        std::string err;
        json11::Json mJson = json11::Json::parse(message, err);
        if(!err.empty()){
            qDebug()<<"parse json error:"<<err.c_str();
            return;
        }

        // 判断来源
        if(!mJson["from"].is_null() && !mJson["msg"].is_null()){
            if(mJson["from"].string_value() == dialogistName.toStdString()){
                // 如果来源是对话者 则显示
                ui->person2TextBrowser->setText(QString::fromStdString(mJson["msg"].string_value()));
                /*
                // 设置光标位置
                int nCurpos = mJson["cursor"].is_number() ? mJson["cursor"].int_value() : 0;
                QTextCursor tc;
                tc.setPosition(nCurpos);
                qDebug()<<"get cursor:"<<nCurpos;
                ui->person2TextBrowser->setTextCursor(tc);
                */
            }
        }

        // 打印json，判断键是否存在
        // qDebug()<<"json:"<<mJson.dump().c_str();
        // qDebug()<<"key msg:"<<!mJson["msg"].is_null()<<", msg2:"<<!mJson["msg2"].is_null();
    });

    connect(mClient, &QMqttClient::pingResponseReceived, this, [this]() {
        const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(" PingResponse")
                    + QLatin1Char('\n');
        qDebug()<<"ping";
    });

    // redis
    RedisConnect::Setup(util::readIni(ENV_PATH, "redis/ip").toString().toStdString(),
                        util::readIni(ENV_PATH, "redis/port").toInt(),
                        util::readIni(ENV_PATH, "redis/password").toString().toStdString()); // 初始化连接池
    qDebug()<<RedisConnect::CanUse();
    redis = RedisConnect::Instance(); // 获取一个连接
    if(redis==NULL){
        qDebug()<< QString("redis con failed, please check %1 file.").arg(ENV_PATH);
    }



    // textChanged 事件获取输入信息
    connect(ui->person1TextBrowser, &QTextBrowser::textChanged, this, &Widget::slot_getInputMsg);
    connect(ui->person1TextBrowser, &QTextBrowser::cursorPositionChanged, this, &Widget::slot_getInputMsg);
}

Widget::~Widget()
{
    delete refreshRoomTimer;
    delete refreshRoomStateTimer;
    delete ui;
}

// 连接
void Widget::on_conBtn_clicked(bool checked)
{
    if(checked){
        // 连接
        // 判断用户名
        if(ui->nameEdit->text().isEmpty()){
            QMessageBox::critical(this, "error", "名称不能为空");
            ui->conBtn->setChecked(false);
            return;
        }
        name = ui->nameEdit->text();

        qDebug()<<"con";
        mClient->connectToHost();
    }else{
        // 断开连接
        qDebug()<<"discon";
        mClient->disconnectFromHost();
    }
}

// 刷新房间列表
void Widget::refreshRoomList(){
    // 获取房间列表
    std::vector<std::string> rooms = getRooms();
    QString roomName;

    ui->roomComboBox->clear();
    for(int i=0; i<rooms.size(); i++){
        roomName = QString::fromStdString(rooms[i]);
        if(!roomName.startsWith("room_")){
            continue;
        }
        roomName.remove(0, strlen("room_"));
        ui->roomComboBox->addItem(roomName);
    }
}

// 获取房间列表
std::vector<std::string> Widget::getRooms(){
    vector<string> rooms;

    if(redis!=NULL){
        redis->keys(rooms, "room_*"); // every room begin with 'room_'
    }

    return rooms;


}

// 加入和退出房间按钮
void Widget::on_roomBtn_clicked(bool checked)
{


    if(checked){
        // 进入房间
        QString room = ui->roomComboBox->currentText();
        if(!joinRoom(room)){
            ui->roomBtn->setChecked(false);
            return;
        }else{
            // 成功后失能下拉框
            ui->roomComboBox->setEnabled(false);
        }

    }else{
        // 离开房间
        leaveRoom();

        ui->roomComboBox->setEnabled(true);
    }

}

bool Widget::joinRoom(QString roomName){
    // 判断mqtt连接
    qDebug()<<state;
    if(state!=QMqttClient::Connected){
        QMessageBox::critical(this, "error", "请先连接服务器");
        return false;
    }
    // 判断房间名
    if(ui->roomComboBox->currentText().isEmpty()){
        QMessageBox::critical(this, "error", "房间名称不能为空");
        return false;
    }
    // 进入房间
    if(redis!=NULL){

        currentRoomName = "room_"+roomName;


        if(!getRoomState()){
            return false;
        }

        // 房间中写入人员
        if( !redis->zadd(currentRoomName.toStdString(), name.toStdString(), 1)){
            QMessageBox::critical(this, "error", "失败");
            return false;
        }
        redis->expire(currentRoomName.toStdString(), 60);

        // 聊天界面设置
        // 显示聊天框
        ui->RoomGroupBox->show();
        // 填充发送者信息
        ui->person1Label->setText(name);

        // 订阅房间消息
        auto subscription = mClient->subscribe(currentRoomName);
        if (!subscription) {
            qDebug()<<"sub failed";
        }

        refreshRoomStateTimer->start();

        return true;
    }
}

bool Widget::getRoomState(){
    // 判断房间的人数（除自己外，一人）
    qDebug()<<"test"<<currentRoomName;currentRoomName;
    std::vector<std::string> memberArr, memberOther;
    redis->zrange(memberArr, currentRoomName.toStdString(), 0, 100);
    for(int i=0; i<memberArr.size(); i++){
        qDebug()<<"omember: "<<i<< " "<< memberArr[i].c_str();
        if(memberArr[i] == name.toStdString()){
            /*
            QMessageBox::critical(this, "error", "同名用户已经登入");
            refreshRoomStateTimer->stop();
            return false;
            */
            continue;
        }
        qDebug()<<"add";
        memberOther.push_back(memberArr[i]);
    }
    if(memberOther.size()==0){
        return true;
    }else if(memberOther.size()==1){
        // 如果有人，则获取对方名称
        dialogistName = memberOther[0].c_str();

        // 显示对方信息
        if(!dialogistName.isEmpty()){
            ui->person2Label->setText(dialogistName);
        }

        return true;
    }else{
        // todo 目前只允许两个人
        QMessageBox::critical(this, "error", "房间已满，请更换其他房间");
        refreshRoomStateTimer->stop();
        return false;
    }

    return false;
}

bool Widget::leaveRoom(){

    // 隐藏聊天界面
    ui->RoomGroupBox->hide();
    ui->person1Label->setText("");
    ui->person2Label->setText("");

    // 删除房间的用户信息
    if(redis!=NULL && !currentRoomName.isEmpty()){
        redis->zrem(currentRoomName.toStdString(), name.toStdString());
        qDebug()<<"zrem "<<currentRoomName<<"_"<<name;
    }

    // 删除订阅
    mClient->unsubscribe(currentRoomName);

    currentRoomName = "";
    dialogistName = "";
    return true;
}

void Widget::slot_getInputMsg(){
    QString text = ui->person1TextBrowser->toPlainText();

    // 获取光标位置
    QTextCursor tc = ui->person1TextBrowser->textCursor();
    int nCurpos = tc.position();
    qDebug()<<"line:"<<nCurpos<<","<<text;


    // 发送消息
    if(!name.isEmpty() && !dialogistName.isEmpty()){
        sendMsg(currentRoomName, name, text, nCurpos);
    }



}

// 发送mqtt消息
bool Widget::sendMsg(QString topic, QString username, QString msg, int cursorPosition){
    json11::Json::object mJsonMap;
    mJsonMap["from"] = username.toStdString().c_str();
    mJsonMap["msg"] = msg.toStdString().c_str();
    mJsonMap["typing"] = true;
    mJsonMap["cursor"] = cursorPosition;
    std::string message = ((json11::Json)mJsonMap).dump();
    qDebug()<<topic<<"_"<<message.c_str();

    if (mClient->publish(topic, message.c_str()) == -1)
        qDebug()<< QLatin1String("Could not publish message");

}


