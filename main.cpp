#include <QCoreApplication>

#include "pushserver.h"
#include "GlobalSettings.h"
#include "logger.h"

#include <csignal>

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

    Logger::setup();

    PushServer server(GlobalConfig::DEFAULT_PORT);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    return a.exec();
}
