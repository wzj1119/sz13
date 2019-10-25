#include "log.h"


// 日志回调函数
void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();

    QString text;
    switch(type)
    {
    case QtInfoMsg:
        text = QString("Info:");
        break;
    case QtDebugMsg:
        text = QString("Debug:");
        break;

    case QtWarningMsg:
        text = QString("Warning:");
        break;

    case QtCriticalMsg:
        text = QString("Critical:");
        break;

    case QtFatalMsg:
        text = QString("Fatal:");
    }

    QString context_info = QString("File:[%1] Line:[%2]").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
    QString current_date = QString("[%1]").arg(current_date_time);
    QString message = QString("%1 %2 %3 %4").arg(text).arg(current_date).arg(context_info).arg(msg);

    QDate date = QDate::currentDate();

    QString strFile =QCoreApplication::applicationDirPath() + "/log/" + date.toString(Qt::ISODate) + "_" + "log.txt";
    QFile file(strFile);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << message << "\r\n";
    file.flush();
    file.close();

    std::cout << message.toStdString() << std::endl;
    mutex.unlock();

}
