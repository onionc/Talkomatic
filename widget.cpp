#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // mqtt
    mClient = new QMqttClient(this);
    mClient->setHostname("c3e36aae.cn-shenzhen.emqx.cloud"); // todo: 参数放配置文件
    mClient->setPort(11733);
    mClient->setUsername("chad");
    mClient->setPassword("123456");

    // 连接

    mClient->connectToHost();

    /*
     if (mClient->state() == QMqttClient::Disconnected) {
        qDebug()<<"con ok";
    } else {
        qDebug()<<"con failed"<<mClient->state();
    }
    */
    // 判断连接

    connect(mClient, &QMqttClient::stateChanged, this, [this](){
        qDebug()<<"state:"<<mClient->state();

        QString topic("chadTest");
        // 发送一条消息
        if (mClient->publish(topic+"/123", "test") == -1)
            qDebug()<< QLatin1String("Could not publish message");

        // 订阅
        auto subscription = mClient->subscribe(topic+"/#");
        if (!subscription) {
            qDebug()<<"sub failed";
        }
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
    });

    connect(mClient, &QMqttClient::pingResponseReceived, this, [this]() {
        const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(" PingResponse")
                    + QLatin1Char('\n');
        qDebug()<<"ping";
    });


}

Widget::~Widget()
{
    delete ui;
}
