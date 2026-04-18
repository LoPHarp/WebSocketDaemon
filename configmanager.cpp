#include "configmanager.h"
#include "GlobalSettings.h"
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <iostream>

using namespace std;

bool ConfigManager::parseSslPaths(const QCoreApplication& app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("C++ WebSocket Daemon");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("path", "Path to directory containing fullchain.pem and privkey.pem (optional)");

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (!args.isEmpty())
    {
        QString certsDir = args.first();
        QDir dir(certsDir);

        if (!dir.exists())
        {
            cerr << "[ERROR] Directory does not exist: " << certsDir.toStdString() << endl;
            return false;
        }

        GlobalConfig::CERT_FILE_PATH = dir.absoluteFilePath("fullchain.pem").toStdString();
        GlobalConfig::KEY_FILE_PATH = dir.absoluteFilePath("privkey.pem").toStdString();
    }

   if (!QFileInfo::exists(QString::fromStdString(GlobalConfig::CERT_FILE_PATH)) ||
        !QFileInfo::exists(QString::fromStdString(GlobalConfig::KEY_FILE_PATH)))
    {
        cerr << "[ERROR] SSL certificates not found!" << endl;
        cerr << "Expected paths:" << endl;
        cerr << " - " << GlobalConfig::CERT_FILE_PATH << endl;
        cerr << " - " << GlobalConfig::KEY_FILE_PATH << endl;
        return false;
    }

    return true;
}
