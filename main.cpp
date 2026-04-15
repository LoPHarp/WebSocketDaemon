#include <QCoreApplication>
#include "pushserver.h"
#include "GlobalSettings.h"

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    PushServer server(GlobalConfig::DEFAULT_PORT);

    return a.exec();
}
