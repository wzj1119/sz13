#ifndef LOG_H
#define LOG_H
#include <QCoreApplication>
#include <iostream>
#include <QtDebug>
#include <QString>
#include <QFile>
#include <QMessageLogger>
#include <qlogging.h>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>


void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

#endif // LOG_H
