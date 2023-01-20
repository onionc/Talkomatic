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

    // mqtt
    mClient = new QMqttClient(this);
    mClient->setHostname(util::readIni(ENV_PATH, "mqtt/hostname").toString()); // todo: 参数放配置文件
    mClient->setPort(util::readIni(ENV_PATH, "mqtt/port").toInt());
    mClient->setUsername(util::readIni(ENV_PATH, "mqtt/username").toString());
    mClient->setPassword(util::readIni(ENV_PATH, "mqtt/password").toString());


    state = QMqttClient::Disconnected;
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

        login();
        //QString topic("chadTest");
        // 发送一条消息
        /*
        std::string message = R"(
            {
                "from":"A",
                "to":"B",
                "msg:"hola",
                "typing":true
            }
                              )", err;
        */
        /*
        json11::Json::object mJsonMap;
        mJsonMap["from"] = this->name.toStdString().c_str();
        //mJsonMap["to"] = "B";
        mJsonMap["msg"] = "hola";
        mJsonMap["typing"] = true;
        std::string message = ((json11::Json)mJsonMap).dump();

        if (mClient->publish(topic+"/server", message.c_str()) == -1)
            qDebug()<< QLatin1String("Could not publish message");

        // 订阅
        qDebug()<<"topic name:"<<topic+"/"+this->name;
        auto subscription = mClient->subscribe(topic+"/"+this->name);
        if (!subscription) {
            qDebug()<<"sub failed";
        }
        */
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
}

Widget::~Widget()
{
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

// 登入服务器
void Widget::login(){
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
        qDebug()<<roomName;
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

// 进入房间
void Widget::on_roomBtn_clicked(bool checked)
{
    if(checked){
        // 进入房间
        // 判断mqtt连接
        qDebug()<<state;
        if(state!=QMqttClient::Connected){
            QMessageBox::critical(this, "error", "请先连接服务器");
            ui->roomBtn->setChecked(false);
            return;
        }
        // 判断房间名
        if(ui->roomComboBox->currentText().isEmpty()){
            QMessageBox::critical(this, "error", "房间名称不能为空");
            ui->roomBtn->setChecked(false);
            return;
        }
        // 进入房间
        if(redis!=NULL){
            QString room = ui->roomComboBox->currentText();
            std::string r2 = "room_"+room.toStdString();

            // 房间中写入人员
            if( !redis->zadd(r2, name.toStdString(), 1)){
                QMessageBox::critical(this, "error", "失败");
                ui->roomBtn->setChecked(false);
                return;
            }
            redis->expire(r2, 60);
            // todo: 进入聊天界面
        }
    }else{
        // 离开房间

    }

}
