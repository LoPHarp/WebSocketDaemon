#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

namespace GlobalConfig
{
    constexpr unsigned short DEFAULT_PORT = 8080;   // Порт головного серверу
    constexpr int MAX_CONNECTIONS = 1000;           // MAX кількість користувачів

    constexpr int MAX_PAYLOAD_SIZE = 8192;          // MAX розмір інформації (КБ)
    constexpr int MAX_MESSAGES_PER_SECOND = 5;      // MAX повідомлень за 1 секунду (відключення при перевищенні, захист від спаму)
    constexpr int PING_INTERVAL_MS = 30000;         // Частота перевірки з'єднання (мс)

    // Шляхи до SSL сертифікатів (відносні або абсолютні)
    constexpr const char* CERT_FILE_PATH = "websocket.crt";
    constexpr const char* KEY_FILE_PATH = "websocket.key";

    // НАЛАШТУВАННЯ ЛОГЕРА
    constexpr bool ENABLE_FILE_LOGGING = false;              // Писати у файл чи ні?
    constexpr const char* LOG_FILE_PATH = "server.log";     // Назва файлу логів
}

#endif
