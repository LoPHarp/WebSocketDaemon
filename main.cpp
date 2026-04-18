#include <QCoreApplication>
#include <csignal>

#include "pushserver.h"
#include "GlobalSettings.h"
#include "logger.h"
#include "configmanager.h"

using namespace std;

static PushServer* globalServerPtr = nullptr;

void signalHandler(int sig)
{
    if (globalServerPtr)
    {
        globalServerPtr->shutdown();
        qApp->quit();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("WebSocketDaemon");

    Logger::setup();

    if (!ConfigManager::parseSslPaths(a))
    {
        return 1;
    }

    PushServer server(GlobalConfig::DEFAULT_PORT);
    globalServerPtr = &server;

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    return a.exec();
}
