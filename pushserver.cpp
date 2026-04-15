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
    m_pingTimer(new QTimer(this))
{
    QSslConfiguration sslConfiguration;
    QFile certFile(QString::fromUtf8(GlobalConfig::CERT_FILE_PATH));
    QFile keyFile(QString::fromUtf8(GlobalConfig::KEY_FILE_PATH));

    if (certFile.open(QIODevice::ReadOnly) && keyFile.open(QIODevice::ReadOnly))
    {
        QSslCertificate certificate(&certFile, QSsl::Pem);
        QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
        certFile.close();
        keyFile.close();

        sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfiguration.setLocalCertificate(certificate);
        sslConfiguration.setPrivateKey(sslKey);
        sslConfiguration.setProtocol(QSsl::TlsV1_2OrLater);

        m_pWebSocketServer->setSslConfiguration(sslConfiguration);
    }
    else
        qDebug() << "CRITICAL: Cannot open SSL certificates!";

    if (m_pWebSocketServer->listen(QHostAddress::Any, port))
        qDebug() << "Server listening on port " << port;

    connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &PushServer::onNewConnection);

    connect(m_pingTimer, &QTimer::timeout, this, &PushServer::sendPings);
    m_pingTimer->start(GlobalConfig::PING_INTERVAL_MS);
}

PushServer::~PushServer()
{
    m_pWebSocketServer->close();
    for (auto& [id, socket] : m_clients)
        socket->deleteLater();
}

void PushServer::sendPings()
{
    for (auto& [id, socket] : m_clients)
        socket->ping();
}

void PushServer::onSslErrors(const QList<QSslError>& errors)
{
    for (const auto& error : errors)
        qDebug() << "SSL Error:" << error.errorString();
}

void PushServer::onNewConnection()
{
    QWebSocket* pSocket = m_pWebSocketServer->nextPendingConnection();

    if (m_clients.size() >= GlobalConfig::MAX_CONNECTIONS)
    {
        qDebug() << "SECURITY: Server is full. Rejecting new connection.";
        pSocket->close();
        pSocket->deleteLater();
        return;
    }

    connect(pSocket, &QWebSocket::textMessageReceived, this, &PushServer::processIncomingMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &PushServer::socketDisconnected);

    QUrl url = pSocket->requestUrl();
    QUrlQuery query(url);
    int userId = query.queryItemValue("id").toInt();

    if (userId > 0)
    {
        m_clients[userId] = pSocket;
        pSocket->setProperty("userId", userId);

        qDebug() << "User" << userId << "connected! Total:" << m_clients.size();
    }
    else
    {
        qDebug() << "Connection rejected: No ID provided";
        pSocket->close();
        pSocket->deleteLater();
    }
}

void PushServer::processIncomingMessage(const QString& message)
{
    QWebSocket* pSender = qobject_cast<QWebSocket*>(sender());
    if (!pSender) return;

    if (message.length() > GlobalConfig::MAX_PAYLOAD_SIZE)
    {
        qDebug() << "SECURITY ALERT: Payload too large from user" << pSender->property("userId").toInt() << ". Dropping connection.";
        pSender->close();
        return;
    }

    int senderId = pSender->property("userId").toInt();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if(parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        qDebug() << "JSON Parse Error from user" << senderId << ":" << parseError.errorString();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString action = jsonObj["action"].toString();

    if (action == "broadcast")
    {
        QString text = jsonObj["text"].toString();
        UserPushToAllUsers(senderId, text);
    }
    else if (action == "send_to_group")
    {
        QJsonArray targetsArray = jsonObj["targets"].toArray();
        QString text = jsonObj["text"].toString();
        QString formattedText = QString("[Group Msg from %1]: %2").arg(senderId).arg(text);

        for (int i = 0; i < targetsArray.size(); ++i)
        {
            int targetId = targetsArray[i].toInt();
            ServerPushToUser(targetId, formattedText);
        }
        qDebug() << "Routed group message from" << senderId << "to" << targetsArray.size() << "users";
    }
    else
        qDebug() << "Unknown action received:" << action;
}

void PushServer::UserPushToAllUsers(int senderId, const QString& message)
{
    for (auto& [id, socket] : m_clients) {
        if (id != senderId) {
            socket->sendTextMessage("User " + QString::number(senderId) + " says: " + message);
        }
    }
}

void PushServer::ServerPushToUser(int targetUserId, const QString& message)
{
    auto it = m_clients.find(targetUserId);
    if (it != m_clients.end()) {
        it->second->sendTextMessage(message);
    }
}

void PushServer::socketDisconnected()
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());

    if (pClient)
    {
        int userId = pClient->property("userId").toInt();
        m_clients.erase(userId);
        pClient->deleteLater();
        qDebug() << "Client disconnected. Total clients:" << m_clients.size();
    }
}
