#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

namespace GlobalConfig
{
    constexpr unsigned short DEFAULT_PORT = 8080;   // Порт головного серверу
    constexpr int MAX_CONNECTIONS = 1000;           // MAX кількість користувачів

    constexpr int MAX_PAYLOAD_SIZE = 8192;          // MAX розмір інформації
    constexpr int PING_INTERVAL_MS = 30000;         // Частота перевірки з'єднання

    // Шляхи до SSL сертифікатів (відносні або абсолютні)
    constexpr const char* CERT_FILE_PATH = "websocket.crt";
    constexpr const char* KEY_FILE_PATH = "websocket.key";
}

#endif
