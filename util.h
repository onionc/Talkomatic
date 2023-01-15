#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <fstream>
#include <QDateTime>
#include <QDir>
#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include <QSettings>
#include <QTextCodec>

namespace util{

    // 获取日期或日期时间  name=true 可作为文件名使用的字符串
    QString getDatetime(bool onlyDate=false, bool name=false);

    // 创建目录，多级以/分隔
    QString createDir(QString path);

    // 日志初始化
    void logInit(long maxSize=1024*1024*10);

    // 写文件
    class WriteFile{
        private:
            QString filePath;
        public:
            std::ofstream *ostrm;
            WriteFile();
            ~WriteFile();

            // 打开文件，如果是本目录，可用"./"表示或只写文件名称
            bool open(QString path);
            void close();

            // 写数据
            void write(QString s);
            // 返回文件名
            QString getFilePath();
    };


    // 写入ini文件
    void writeIni(QString saveFilename, QString key, QVariant value);
    // 读取ini
    QVariant readIni(QString readFilename, QString key);
}


#endif // UTIL_H
