#include "logger.h"
#include "GlobalSettings.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <iostream>

using namespace std;

static QMutex logMutex;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    logMutex.lock();

    QString txt;
    switch (type)
    {
    case QtDebugMsg:    txt = QStringLiteral("[DEBUG]"); break;
    case QtInfoMsg:     txt = QStringLiteral("[INFO]"); break;
    case QtWarningMsg:  txt = QStringLiteral("[WARN]"); break;
    case QtCriticalMsg: txt = QStringLiteral("[CRITICAL]"); break;
    case QtFatalMsg:    txt = QStringLiteral("[FATAL]"); break;
    }

    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");

    QString file = context.file ? QString(context.file) : "Unknown";
    int lastSlash = file.lastIndexOf('/');
    int lastBackslash = file.lastIndexOf('\\');
    int slashIndex = qMax(lastSlash, lastBackslash);
    if (slashIndex != -1) file = file.mid(slashIndex + 1);

    QString formattedMessage = QString("%1 %2 [%3:%4] %5")
                                   .arg(currentDateTime)
                                   .arg(txt)
                                   .arg(file)
                                   .arg(context.line)
                                   .arg(msg);

    cout << formattedMessage.toStdString() << endl;

    if (GlobalConfig::ENABLE_FILE_LOGGING)
    {
        QFile outFile(QString::fromUtf8(GlobalConfig::LOG_FILE_PATH));
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            QTextStream ts(&outFile);
            ts << formattedMessage << "\n";
            outFile.close();
        }
    }

    logMutex.unlock();
}

void Logger::setup()
{
    qInstallMessageHandler(customMessageHandler);
}
