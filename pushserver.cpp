#include "pushserver.h"
#include "GlobalSettings.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslConfiguration>

using namespace std;

PushServer::PushServer(quint16 port, QObject *parent) : QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Push Server"), QWebSocketServer::SecureMode, this)),
    m_pingTimer(new QTimer(this)),
    m_networkManager(new QNetworkAccessManager(this))
{
    QSslConfiguration sslConfiguration;
    QFile certFile(QString::fromStdString(GlobalConfig::CERT_FILE_PATH));
    QFile keyFile(QString::fromStdString(GlobalConfig::KEY_FILE_PATH));

    if (!certFile.open(QIODevice::ReadOnly))
        qCritical() << "Failed to open certificate (" << GlobalConfig::CERT_FILE_PATH.c_str() << "). Reason:" << certFile.errorString();
    else if (!keyFile.open(QIODevice::ReadOnly))
    {
        qCritical() << "Failed to open private key (" << GlobalConfig::KEY_FILE_PATH.c_str() << "). Reason:" << keyFile.errorString();
        certFile.close();
    }
    else
    {
        QSslCertificate certificate(&certFile, QSsl::Pem);
        QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
        certFile.close();
        keyFile.close();

        if (certificate.isNull() || sslKey.isNull())
            qCritical() << "Files opened, but certificates are corrupted or invalid PEM format!";
        else
        {
            sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConfiguration.setLocalCertificate(certificate);
            sslConfiguration.setPrivateKey(sslKey);
            sslConfiguration.setProtocol(QSsl::TlsV1_2OrLater);

            m_pWebSocketServer->setSslConfiguration(sslConfiguration);
            qInfo() << "Certificates loaded securely from config.";
        }
    }
    connect(m_pWebSocketServer, &QWebSocketServer::sslErrors, this, &PushServer::onSslErrors);

    if (m_pWebSocketServer->listen(QHostAddress::Any, port))
        qInfo() << "Server listening on port " << port;

    connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &PushServer::onNewConnection);

    connect(m_pingTimer, &QTimer::timeout, this, &PushServer::sendPings);
    m_pingTimer->start(GlobalConfig::PING_INTERVAL_MS);
}

PushServer::~PushServer()
{
    m_pWebSocketServer->close();
    for (auto& [role, socketList] : m_clients)
        for (QWebSocket* socket : socketList)
            socket->deleteLater();
}

void PushServer::shutdown()
{
    qInfo() << "Shutting down server gracefully...";

    QJsonObject rebootObj;
    rebootObj["action"] = "server_reboot";
    rebootObj["message"] = "Server is restarting for maintenance. Please wait.";
    QString msg = QJsonDocument(rebootObj).toJson(QJsonDocument::Compact);

    for (auto& [role, socketList] : m_clients)
        for (QWebSocket* socket : socketList)
            socket->sendTextMessage(msg);

    m_pWebSocketServer->close();

    qInfo() << "Broadcasted reboot message to all clients. Exit.";
}

void PushServer::sendPings()
{
    for (auto& [role, socketList] : m_clients)
        for (QWebSocket* socket : socketList)
            socket->ping();
}

void PushServer::onSslErrors(const QList<QSslError>& errors)
{
    for (const auto& error : errors)
        qWarning() << "SSL Error:" << error.errorString();
}

void PushServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    QUrl url = pSocket->requestUrl();
    QUrlQuery query(url);
    QString tokenParam = QString::fromStdString(GlobalConfig::HTTP_TOKEN_PARAM);
    QString token = query.queryItemValue(tokenParam);

    if (token.isEmpty())
    {
        qWarning() << "Connection rejected: No token provided.";
        pSocket->close(QWebSocketProtocol::CloseCodePolicyViolated, "Token missing");
        pSocket->deleteLater();
        return;
    }

    qInfo() << "New handshake request with token:" << token;

    QUrl authUrl(QString::fromStdString(GlobalConfig::PHP_AUTH_URL));
    QUrlQuery authQuery;
    authQuery.addQueryItem(tokenParam, token);
    authUrl.setQuery(authQuery);

    QNetworkRequest request(authUrl);
    request.setTransferTimeout(GlobalConfig::HTTP_TIMEOUT_MS);

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, pSocket, token]()
            {
                if (reply->error() == QNetworkReply::NoError)
                {
                    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                    QJsonObject obj = doc.object();

                    QString status = obj[QString::fromStdString(GlobalConfig::JSON_AUTH_STATUS_KEY)].toString();

                    if (status == "ok")
                    {
                        QString role = obj[QString::fromStdString(GlobalConfig::JSON_AUTH_ROLE_KEY)].toString();
                        int factoryId = obj[QString::fromStdString(GlobalConfig::JSON_AUTH_FACTORY_KEY)].toInt();
                        int subfactoryId = obj[QString::fromStdString(GlobalConfig::JSON_AUTH_SUB_KEY)].toInt();
                        QString userName = obj[QString::fromStdString(GlobalConfig::JSON_AUTH_NAME_KEY)].toString();

                        pSocket->setProperty("userId", token);
                        pSocket->setProperty("role", role);
                        pSocket->setProperty("factoryId", factoryId);
                        pSocket->setProperty("subfactoryId", subfactoryId);

                        m_clients[role].append(pSocket);

                        connect(pSocket, &QWebSocket::textMessageReceived, this, &PushServer::processIncomingMessage);
                        connect(pSocket, &QWebSocket::disconnected, this, &PushServer::socketDisconnected);

                        qInfo() << "User" << userName << "(" << role << ") authorized for factory" << factoryId;
                    }
                    else
                    {
                        qWarning() << "PHP rejected token:" << token;
                        pSocket->close(QWebSocketProtocol::CloseCodePolicyViolated, "Invalid token");
                        pSocket->deleteLater();
                    }
                }
                else
                {
                    qCritical() << "Auth error (PHP unreachable):" << reply->errorString();
                    pSocket->close(QWebSocketProtocol::CloseCodeBadOperation, "Auth service unavailable");
                    pSocket->deleteLater();
                }

                reply->deleteLater();
            });
}

void PushServer::processIncomingMessage(const QString& message)
{
    QWebSocket* pSender = qobject_cast<QWebSocket*>(sender());
    if (!pSender) return;

    QString senderIdStr = pSender->property("userId").toString();
    int senderFactoryId = pSender->property("factoryId").toInt();
    int senderSubfactoryId = pSender->property("subfactoryId").toInt();

    if (message.length() > GlobalConfig::MAX_PAYLOAD_SIZE)
    {
        qWarning() << "SECURITY ALERT: Payload too large from" << senderIdStr << ". Dropping connection.";
        pSender->close();
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 lastResetTime = pSender->property("lastResetTime").toLongLong();
    int msgCount = pSender->property("msgCount").toInt();

    if (currentTime - lastResetTime > 1000)
    {
        pSender->setProperty("msgCount", 1);
        pSender->setProperty("lastResetTime", currentTime);
    }
    else
    {
        msgCount++;
        if (msgCount > GlobalConfig::MAX_MESSAGES_PER_SECOND)
        {
            qWarning() << "SECURITY ALERT: Rate limit exceeded by" << senderIdStr << "(Spam). Disconnecting.";
            pSender->close();
            return;
        }
        pSender->setProperty("msgCount", msgCount);
    }

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Invalid JSON from" << senderIdStr << ":" << parseError.errorString();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString action = jsonObj[QString::fromStdString(GlobalConfig::JSON_ACTION_KEY)].toString();

    if (senderIdStr == QString::fromStdString(GlobalConfig::ROLE_PHP_BACKEND) || senderFactoryId == 0)
    {
        QString factoryKey = QString::fromStdString(GlobalConfig::JSON_FACTORY_KEY);
        QString subfactoryKey = QString::fromStdString(GlobalConfig::JSON_SUBFACTORY_KEY);

        if (jsonObj.contains(factoryKey))
            senderFactoryId = jsonObj[factoryKey].toInt();

        if (jsonObj.contains(subfactoryKey))
            senderSubfactoryId = jsonObj[subfactoryKey].toInt();
    }

    if (action == QString::fromStdString(GlobalConfig::ACTION_INSERT_ZAYAVKA))
    {
        QString rawPayload = QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact));
        QJsonArray usersArray = jsonObj[QString::fromStdString(GlobalConfig::JSON_USER_KEY)].toArray();

        forwardToPhpAndBroadcast(senderIdStr, senderFactoryId, senderSubfactoryId, jsonObj, usersArray, rawPayload);
    }
    else if (action == QString::fromStdString(GlobalConfig::ACTION_BROADCAST))
    {
        if (senderIdStr == QString::fromStdString(GlobalConfig::ROLE_ADMIN) || senderIdStr == QString::fromStdString(GlobalConfig::ROLE_PHP_BACKEND))
        {
            QString text = jsonObj[QString::fromStdString(GlobalConfig::JSON_TEXT_KEY)].toString();
            UserPushToAllUsers(senderIdStr, text, senderFactoryId, senderSubfactoryId);
            qInfo() << "ADMIN" << senderIdStr << "executed broadcast for factory" << senderFactoryId;
        }
        else
        {
            qWarning() << "SECURITY ALERT: Unauthorized broadcast attempt from" << senderIdStr;
            pSender->close(QWebSocketProtocol::CloseCodePolicyViolated, "Unauthorized action");
        }
    }
    else
        qWarning() << "Unknown action received:" << action;
}

void PushServer::ServerPushToUserOrRole(const QString& targetIdOrRole, const QString& message, int senderFactoryId, int senderSubfactoryId)
{
    auto it = m_clients.find(targetIdOrRole);
    if (it != m_clients.end())
    {
        for (QWebSocket* pReceiver : it->second)
        {
            int receiverFactoryId = pReceiver->property("factoryId").toInt();
            int receiverSubfactoryId = pReceiver->property("subfactoryId").toInt();

            if (senderFactoryId != 0 && senderFactoryId != receiverFactoryId)
                continue;

            if (senderSubfactoryId != 0 && receiverSubfactoryId != 0 && senderSubfactoryId != receiverSubfactoryId)
                continue;

            pReceiver->sendTextMessage(message);
        }
    }
}

void PushServer::UserPushToAllUsers(const QString& senderId, const QString& message, int senderFactoryId, int senderSubfactoryId)
{
    for (const auto& [role, socketList] : m_clients)
    {
        for (QWebSocket* pReceiver : socketList)
        {
            if (pReceiver->property("userId").toString() == senderId)
                continue;

            int receiverFactoryId = pReceiver->property("factoryId").toInt();
            int receiverSubfactoryId = pReceiver->property("subfactoryId").toInt();

            if (senderFactoryId != 0 && senderFactoryId != receiverFactoryId)
                continue;

            if (senderSubfactoryId != 0 && receiverSubfactoryId != 0 && senderSubfactoryId != receiverSubfactoryId)
                continue;

            pReceiver->sendTextMessage("User " + senderId + " says: " + message);
        }
    }
}

void PushServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient)
    {
        QString userId = pClient->property("userId").toString();

        if (!userId.isEmpty())
        {
            QUrl url(QString::fromStdString(GlobalConfig::PHP_DISCONNECT_URL));
            if (url.isValid())
            {
                QNetworkRequest request(url);
                request.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromStdString(GlobalConfig::HTTP_CONTENT_TYPE_VAL));
                request.setTransferTimeout(GlobalConfig::HTTP_TIMEOUT_MS);

                QJsonObject jsonObj;
                jsonObj[QString::fromStdString(GlobalConfig::JSON_DISCONNECT_TOKEN_KEY)] = userId;

                QByteArray data = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);

                QNetworkReply* reply = m_networkManager->post(request, data);

                connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);

                qInfo() << "Sent disconnect notification to PHP for user:" << userId;
            }
        }

        for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
        {
            if (it->second.contains(pClient))
            {
                it->second.removeAll(pClient);
                if (it->second.isEmpty())
                    m_clients.erase(it);
                break;
            }
        }

        pClient->deleteLater();
    }
}

void PushServer::forwardToPhpAndBroadcast(const QString& senderIdStr, int senderFactoryId, int senderSubfactoryId, const QJsonObject& jsonObj, const QJsonArray& targetRoles, const QString& rawPayload)
{
    QUrl url(QString::fromStdString(GlobalConfig::PHP_SAVE_URL));

    if (!url.isValid())
    {
        qCritical() << "Invalid SAVE_URL in config! ( " << GlobalConfig::PHP_SAVE_URL.c_str() << " )";
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromStdString(GlobalConfig::HTTP_CONTENT_TYPE_VAL));
    request.setTransferTimeout(GlobalConfig::HTTP_TIMEOUT_MS);

    QByteArray data = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = m_networkManager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, reply, targetRoles, rawPayload, senderIdStr, senderFactoryId, senderSubfactoryId]()
            {
                if (reply->error() == QNetworkReply::NoError)
                {
                    qInfo() << "PHP DB SUCCESS. Routing insert_zayavka from" << senderIdStr;
                    for (int i = 0; i < targetRoles.size(); ++i)
                        ServerPushToUserOrRole(targetRoles[i].toString(), rawPayload, senderFactoryId, senderSubfactoryId);
                }
                else
                {
                    qWarning() << "PHP DB ERROR for" << senderIdStr << ":" << reply->errorString();
                    qDebug() << "Response body:" << reply->readAll();
                }

                reply->deleteLater();
            });
}
