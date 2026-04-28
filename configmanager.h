#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>

class ConfigManager
{
public:
    static bool loadExternalConfig(const QString& path = "config.json");
    static void printCurrentConfig();
};

#endif // CONFIGMANAGER_H
