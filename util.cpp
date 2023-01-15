#include "util.h"

#include "util.h"

static bool smallEndian = true;

// 获取日期或日期时间
QString util::getDatetime(bool onlyDate, bool name){
    QDateTime time = QDateTime::currentDateTime();

    QString tmp;
    if(onlyDate){
        if(name){
            tmp = time.toString("yyyy-MM-dd");
        }else{
            tmp = time.toString("yyyy_MM_dd");
        }

    }else{
        if(name){
            // 可作为文件名使用的字符串
            tmp = time.toString("yyyy_MM_dd_hh_mm_ss");
        }else{
            tmp = time.toString("yyyy-MM-dd hh:mm:ss");
        }
    }

    return tmp;
}


// 创建目录
QString util::createDir(QString path){
    QDir dir(path);

    if(dir.exists(path)){
        return path;
    }

    int i = path.lastIndexOf('/');
    QString parentDir="", dirName="";

    if(i>0){
        // 有多级目录
        parentDir = createDir(path.mid(0, i));
    }

    // 创建目录
    dirName = path.mid(i+1);
    QDir  parentPath(parentDir);
    if(!dirName.isEmpty()){
        parentPath.mkpath(dirName);
    }


    return i>0 ? parentDir+"/"+dirName : dirName;
}

// 日志初始化
void util::logInit(long maxSize){
    // 多线程不安全
    static bool f = false;
    if(!f){
        // 创建目录
        QString dir = "./log/";
        dir = createDir(dir);

        dir+=getDatetime(true)+".log";

        plog::init(plog::debug, dir.toStdString().c_str(), maxSize, 1);
        f = true;
    }

}


// 创建文件
util::WriteFile::WriteFile():ostrm(NULL), filePath(""){}
util::WriteFile::~WriteFile(){close();}

void util::WriteFile::close(){
    if(ostrm && ostrm->is_open()){
        ostrm->close();
    }
}

bool util::WriteFile::open(QString path){
    // 拆分目录和文件名
    int i = path.lastIndexOf('/');
    QString dirName="",filename="";

    filename = path.mid(i+1);
    if(filename.isEmpty()){
        return false;
    }

    if(i>0){
        // 有目录
        dirName = createDir(path.mid(0, i));
        filePath = dirName+"/"+filename;
    }else{
        filePath = filename;
    }

    // 创建文件
    ostrm = new std::ofstream(filePath.toStdString(), std::ios::app|std::ios::binary);
    if(ostrm->is_open()){
        return true;
    }

    return false;
}

void util::WriteFile::write(QString s){
    if(ostrm->is_open()){
        *ostrm << s.toStdString();
    }
}

QString util::WriteFile::getFilePath(){
    return filePath;
}


// 写入ini文件
void util::writeIni(QString saveFilename, QString key, QVariant value){

    QSettings write(saveFilename, QSettings::IniFormat);
    write.setIniCodec(QTextCodec::codecForName("UTF-8"));
    //write.clear();

    write.setValue(key, value);
}


// 读取ini
QVariant util::readIni(QString readFilename, QString key){
    // 打开ini文件
    QSettings read(readFilename, QSettings::IniFormat);
    return read.value(key);
}
