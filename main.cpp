#include <QCoreApplication>
#include "pushserver.h"
#include "GlobalSettings.h"
#include "logger.h"

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Logger::setup();

    PushServer server(GlobalConfig::DEFAULT_PORT);

    return a.exec();
}
