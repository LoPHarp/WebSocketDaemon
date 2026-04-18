#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QCoreApplication>

class ConfigManager
{
public:
    static bool parseSslPaths(const QCoreApplication& app);
};

#endif // CONFIGMANAGER_H
