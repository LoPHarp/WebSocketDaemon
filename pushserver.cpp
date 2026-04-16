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
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Push Server"), QWebSocketServer::NonSecureMode, this)), //Не забути повернути SecureMode
    m_pingTimer(new QTimer(this))
{
    /* ТИМЧАСОВО ВІДКЛЮЧЕНО ДЛЯ ЛОКАЛЬНОГО ТЕСТУВАННЯ
    QSslConfiguration sslConfiguration;
    QFile certFile(QString::fromUtf8(GlobalConfig::CERT_FILE_PATH));
    QFile keyFile(QString::fromUtf8(GlobalConfig::KEY_FILE_PATH));

    if (!certFile.open(QIODevice::ReadOnly))
        qCritical() << "Failed to open certificate (" << GlobalConfig::CERT_FILE_PATH << "). Reason:" << certFile.errorString();
    else if (!keyFile.open(QIODevice::ReadOnly))
    {
        qCritical() << "Failed to open private key (" << GlobalConfig::KEY_FILE_PATH << "). Reason:" << keyFile.errorString();
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
            qInfo() << "Certificates loaded securely.";
        }
    }
    connect(m_pWebSocketServer, &QWebSocketServer::sslErrors, this, &PushServer::onSslErrors);
    */

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
    int totalConnections = 0;
    for (const auto& [role, socketList] : m_clients)
        totalConnections += socketList.size();

    QWebSocket* pSocket = m_pWebSocketServer->nextPendingConnection();

    if (totalConnections >= GlobalConfig::MAX_CONNECTIONS)
    {
        qWarning() << "SECURITY: Server is full. Rejecting new connection.";
        pSocket->close();
        pSocket->deleteLater();
        return;
    }

    connect(pSocket, &QWebSocket::textMessageReceived, this, &PushServer::processIncomingMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &PushServer::socketDisconnected);

    QUrl url = pSocket->requestUrl();
    QUrlQuery query(url);
    QString userIdStr = query.queryItemValue("id");

    if (!userIdStr.isEmpty())
    {
        m_clients[userIdStr].append(pSocket);
        pSocket->setProperty("userId", userIdStr);

        pSocket->setProperty("msgCount", 0);
        pSocket->setProperty("lastResetTime", QDateTime::currentMSecsSinceEpoch());

        qInfo() << "User/Role [" << userIdStr << "] connected! Total sockets for this role:" << m_clients[userIdStr].size();
    }
    else
    {
        qWarning() << "Connection rejected: No ID provided";
        pSocket->close();
        pSocket->deleteLater();
    }
}

void PushServer::processIncomingMessage(const QString& message)
{
    QWebSocket* pSender = qobject_cast<QWebSocket*>(sender());
    if (!pSender) return;

    QString senderIdStr = pSender->property("userId").toString();

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

    if(parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        qWarning() << "JSON Parse Error from" << senderIdStr << ":" << parseError.errorString();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString action = jsonObj["action"].toString();

    if (action == "insert_zayavka")
    {
        QString rawPayload = QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact));
        QJsonArray usersArray = jsonObj["user"].toArray();

        for (int i = 0; i < usersArray.size(); ++i)
        {
            QString targetUserOrRole = usersArray[i].toString();
            ServerPushToUserOrRole(targetUserOrRole, rawPayload);
        }
        qInfo() << "Routed insert_zayavka from" << senderIdStr << "to" << usersArray.size() << "roles/users";
    }
    else if (action == "broadcast")
    {
        if (senderIdStr == "admin" || senderIdStr == "php_backend")
        {
            QString text = jsonObj["text"].toString();
            UserPushToAllUsers(senderIdStr, text);
            qInfo() << "ADMIN" << senderIdStr << "executed broadcast.";
        }
        else
        {
            qWarning() << "SECURITY ALERT: Unauthorized broadcast attempt from" << senderIdStr;
            // pSender->close();
        }
    }
    else
        qWarning() << "Unknown action received:" << action;
}

void PushServer::ServerPushToUserOrRole(const QString& targetIdOrRole, const QString& message)
{
    auto it = m_clients.find(targetIdOrRole);
    if (it != m_clients.end())
        for (QWebSocket* socket : it->second)
            socket->sendTextMessage(message);
}

void PushServer::UserPushToAllUsers(const QString& senderId, const QString& message)
{
    for (const auto& [role, socketList] : m_clients)
        for (QWebSocket* socket : socketList)
            if (socket->property("userId").toString() != senderId)
                socket->sendTextMessage("User " + senderId + " says: " + message);
}

void PushServer::socketDisconnected()
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());

    if (pClient)
    {
        QString userIdStr = pClient->property("userId").toString();

        auto it = m_clients.find(userIdStr);
        if (it != m_clients.end())
        {
            it->second.removeAll(pClient);

            if (it->second.isEmpty())
                m_clients.erase(it);
        }

        pClient->deleteLater();
        qInfo() << "Client [" << userIdStr << "] disconnected.";
    }
}
