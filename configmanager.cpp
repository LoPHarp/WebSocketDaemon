#include "configmanager.h"
#include "GlobalSettings.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <iostream>

using namespace std;

bool ConfigManager::loadExternalConfig(const QString& path)
{
    if (!QFileInfo::exists(path))
    {
        cout << "[INFO] External config file not found (" << path.toStdString() << "). Using defaults." << endl;
        return true;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        cerr << "[CRITICAL] Config file exists but cannot be opened: " << path.toStdString() << endl;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        cerr << "[CRITICAL] Syntax error in config.json at offset " << parseError.offset << ": " << parseError.errorString().toStdString() << endl;
        return false;
    }

    if (!doc.isObject())
    {
        cerr << "[CRITICAL] Config JSON must contain a root object {...}" << endl;
        return false;
    }

    QJsonObject obj = doc.object();

    if (obj.contains("port")) GlobalConfig::DEFAULT_PORT = obj["port"].toInt();
    if (obj.contains("max_connections")) GlobalConfig::MAX_CONNECTIONS = obj["max_connections"].toInt();
    if (obj.contains("max_payload_size")) GlobalConfig::MAX_PAYLOAD_SIZE = obj["max_payload_size"].toInt();
    if (obj.contains("max_messages_per_second")) GlobalConfig::MAX_MESSAGES_PER_SECOND = obj["max_messages_per_second"].toInt();
    if (obj.contains("ping_interval_ms")) GlobalConfig::PING_INTERVAL_MS = obj["ping_interval_ms"].toInt();

    if (obj.contains("ssl_cert_path")) GlobalConfig::CERT_FILE_PATH = obj["ssl_cert_path"].toString().toStdString();
    if (obj.contains("ssl_key_path")) GlobalConfig::KEY_FILE_PATH = obj["ssl_key_path"].toString().toStdString();

    if (obj.contains("php_auth_url")) GlobalConfig::PHP_AUTH_URL = obj["php_auth_url"].toString().toStdString();
    if (obj.contains("php_save_url")) GlobalConfig::PHP_SAVE_URL = obj["php_save_url"].toString().toStdString();
    if (obj.contains("php_disconnect_url")) GlobalConfig::PHP_DISCONNECT_URL = obj["php_disconnect_url"].toString().toStdString();
    if (obj.contains("http_timeout_ms")) GlobalConfig::HTTP_TIMEOUT_MS = obj["http_timeout_ms"].toInt();

    if (obj.contains("http_content_type_val")) GlobalConfig::HTTP_CONTENT_TYPE_VAL = obj["http_content_type_val"].toString().toStdString();
    if (obj.contains("http_token_param")) GlobalConfig::HTTP_TOKEN_PARAM = obj["http_token_param"].toString().toStdString();

    if (obj.contains("json_action_key")) GlobalConfig::JSON_ACTION_KEY = obj["json_action_key"].toString().toStdString();
    if (obj.contains("json_user_key")) GlobalConfig::JSON_USER_KEY = obj["json_user_key"].toString().toStdString();
    if (obj.contains("json_text_key")) GlobalConfig::JSON_TEXT_KEY = obj["json_text_key"].toString().toStdString();
    if (obj.contains("json_factory_key")) GlobalConfig::JSON_FACTORY_KEY = obj["json_factory_key"].toString().toStdString();
    if (obj.contains("json_subfactory_key")) GlobalConfig::JSON_SUBFACTORY_KEY = obj["json_subfactory_key"].toString().toStdString();
    if (obj.contains("json_disconnect_token_key")) GlobalConfig::JSON_DISCONNECT_TOKEN_KEY = obj["json_disconnect_token_key"].toString().toStdString();

    if (obj.contains("json_auth_status_key")) GlobalConfig::JSON_AUTH_STATUS_KEY = obj["json_auth_status_key"].toString().toStdString();
    if (obj.contains("json_auth_role_key")) GlobalConfig::JSON_AUTH_ROLE_KEY = obj["json_auth_role_key"].toString().toStdString();
    if (obj.contains("json_auth_factory_key")) GlobalConfig::JSON_AUTH_FACTORY_KEY = obj["json_auth_factory_key"].toString().toStdString();
    if (obj.contains("json_auth_sub_key")) GlobalConfig::JSON_AUTH_SUB_KEY = obj["json_auth_sub_key"].toString().toStdString();
    if (obj.contains("json_auth_name_key")) GlobalConfig::JSON_AUTH_NAME_KEY = obj["json_auth_name_key"].toString().toStdString();

    if (obj.contains("action_insert_zayavka")) GlobalConfig::ACTION_INSERT_ZAYAVKA = obj["action_insert_zayavka"].toString().toStdString();
    if (obj.contains("action_broadcast")) GlobalConfig::ACTION_BROADCAST = obj["action_broadcast"].toString().toStdString();
    if (obj.contains("role_php_backend")) GlobalConfig::ROLE_PHP_BACKEND = obj["role_php_backend"].toString().toStdString();
    if (obj.contains("role_admin")) GlobalConfig::ROLE_ADMIN = obj["role_admin"].toString().toStdString();

    if (obj.contains("enable_file_logging")) GlobalConfig::ENABLE_FILE_LOGGING = obj["enable_file_logging"].toBool();
    if (obj.contains("log_file_path")) GlobalConfig::LOG_FILE_PATH = obj["log_file_path"].toString().toStdString();

    cout << "[INFO] External configuration loaded successfully from " << path.toStdString() << endl;
    return true;
}

void ConfigManager::printCurrentConfig()
{
    QString dump = "\n";
    dump += "==================================================\n";
    dump += "         FINAL SERVER CONFIGURATION DUMP          \n";
    dump += "==================================================\n";

    auto addRow = [](const QString& name, const QVariant& val)
    {
        return QString("%1 %2\n").arg(name, -30).arg(val.toString());
    };

    dump += addRow("Network Port:", GlobalConfig::DEFAULT_PORT);
    dump += addRow("Max Connections:", GlobalConfig::MAX_CONNECTIONS);
    dump += addRow("Max Payload (bytes):", GlobalConfig::MAX_PAYLOAD_SIZE);
    dump += addRow("Messages per second:", GlobalConfig::MAX_MESSAGES_PER_SECOND);
    dump += addRow("Ping Interval (ms):", GlobalConfig::PING_INTERVAL_MS);

    dump += "\n--- SSL SETTINGS ---\n";
    dump += addRow("Cert Path:", QString::fromStdString(GlobalConfig::CERT_FILE_PATH));
    dump += addRow("Key Path:", QString::fromStdString(GlobalConfig::KEY_FILE_PATH));

    dump += "\n--- PHP ENDPOINTS ---\n";
    dump += addRow("Auth URL:", QString::fromStdString(GlobalConfig::PHP_AUTH_URL));
    dump += addRow("Save URL:", QString::fromStdString(GlobalConfig::PHP_SAVE_URL));
    dump += addRow("Disconnect URL:", QString::fromStdString(GlobalConfig::PHP_DISCONNECT_URL));
    dump += addRow("HTTP Timeout (ms):", GlobalConfig::HTTP_TIMEOUT_MS);

    dump += "\n--- LOGGING ---\n";
    dump += addRow("File Logging:", GlobalConfig::ENABLE_FILE_LOGGING ? "ENABLED" : "DISABLED");
    dump += addRow("Log Path:", QString::fromStdString(GlobalConfig::LOG_FILE_PATH));

    dump += "==================================================";

    qInfo().noquote() << dump;
}
