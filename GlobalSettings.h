#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <string>

namespace GlobalConfig
{
inline unsigned short DEFAULT_PORT = 8080;   // Порт головного сервера
inline int MAX_CONNECTIONS = 1000;           // Максимальна кількість одночасних підключень
inline int MAX_PAYLOAD_SIZE = 8192;          // Максимальний розмір вхідного пакету (байти)
inline int MAX_MESSAGES_PER_SECOND = 5;      // Ліміт повідомлень за секунду (захист від спаму)
inline int PING_INTERVAL_MS = 30000;         // Інтервал перевірки активності з'єднання (мс)

inline std::string CERT_FILE_PATH = "fullchain.pem"; // Шлях до SSL сертифіката
inline std::string KEY_FILE_PATH = "privkey.pem";    // Шлях до приватного SSL ключа

// --- ЕНДПОІНТИ (URL для зв'язку з PHP) ---
inline std::string PHP_AUTH_URL       = "http://127.0.0.1/api/auth.php";       // Перевірка токена (GET)
inline std::string PHP_SAVE_URL       = "http://127.0.0.1/api/save.php";       // Збереження повідомлення (POST)
inline std::string PHP_DISCONNECT_URL = "http://127.0.0.1/api/disconnect.php"; // Сповіщення про відключення (POST)
inline int HTTP_TIMEOUT_MS            = 5000;                                  // Максимальний час очікування відповіді від PHP (мс)

// --- НАЛАШТУВАННЯ HTTP ЗАГОЛОВКІВ ---
inline std::string HTTP_CONTENT_TYPE_VAL = "application/json"; // Формат даних у тілі POST-запиту
inline std::string HTTP_TOKEN_PARAM      = "token";            // Назва GET-параметра для передачі токена

// --- КЛЮЧІ JSON (ВХІДНІ ТА ВИХІДНІ ПАКЕТИ) ---
inline std::string JSON_ACTION_KEY     = "action";         // Тип дії (наприклад, insert_zayavka)
inline std::string JSON_USER_KEY       = "user";           // Масив отримувачів (ролі або конкретні користувачі)
inline std::string JSON_TEXT_KEY       = "content";        // Текст повідомлення або HTML
inline std::string JSON_FACTORY_KEY    = "id_factory";     // ID підприємства (ЖЕКу)
inline std::string JSON_SUBFACTORY_KEY = "id_subfactory";  // ID підрозділу (субпідрядника)
inline std::string JSON_DISCONNECT_TOKEN_KEY = "token";    // Ключ в JSON, який PHP отримає при розриві з'єднання

// --- ПРОТОКОЛ АВТОРИЗАЦІЇ (Очікувані ключі у відповіді від PHP) ---
inline std::string JSON_AUTH_STATUS_KEY  = "status";       // Статус авторизації ("ok" або "error")
inline std::string JSON_AUTH_ROLE_KEY    = "role";         // Роль користувача
inline std::string JSON_AUTH_FACTORY_KEY = "factory_id";   // ID підприємства (int)
inline std::string JSON_AUTH_SUB_KEY     = "subfactory_id";// ID підрозділу (int або null)
inline std::string JSON_AUTH_NAME_KEY    = "name";         // Ім'я користувача (для логів)

// --- ЗНАЧЕННЯ ДІЙ (Actions) ---
inline std::string ACTION_INSERT_ZAYAVKA  = "insert_zayavka"; // Створення нової заявки
inline std::string ACTION_BROADCAST       = "broadcast";      // Глобальне системне сповіщення

// --- СИСТЕМНІ РОЛІ ---
inline std::string ROLE_PHP_BACKEND       = "php_backend";    // Ідентифікатор самого сервера PHP
inline std::string ROLE_ADMIN             = "admin";          // Ідентифікатор суперкористувача

inline bool ENABLE_FILE_LOGGING = false;             // Увімкнення запису логів у файл
inline std::string LOG_FILE_PATH = "server.log";     // Назва файлу логів
}

#endif
