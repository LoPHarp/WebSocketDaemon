#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <string>

namespace GlobalConfig
{
    constexpr unsigned short DEFAULT_PORT = 8080;   // Порт головного серверу
    constexpr int MAX_CONNECTIONS = 1000;           // MAX кількість користувачів

    constexpr int MAX_PAYLOAD_SIZE = 8192;          // MAX розмір інформації (КБ)
    constexpr int MAX_MESSAGES_PER_SECOND = 5;      // MAX повідомлень за 1 секунду (відключення при перевищенні, захист від спаму)
    constexpr int PING_INTERVAL_MS = 30000;         // Частота перевірки з'єднання (мс)

    // Шляхи до SSL сертифікатів (можуть бути змінені через консоль)
    // За замовчуванням шукають файли у тій самій папці, звідки запущено програму.
    inline std::string CERT_FILE_PATH = "fullchain.pem";
    inline std::string KEY_FILE_PATH = "privkey.pem";

    // НАЛАШТУВАННЯ ЛОГЕРА
    constexpr bool ENABLE_FILE_LOGGING = false;             // Писати у файл чи ні?
    constexpr const char* LOG_FILE_PATH = "server.log";     // Назва файлу логів
}

#endif
