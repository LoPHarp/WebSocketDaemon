#include "pushserver.h"
#include "GlobalSettings.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>

using namespace std;

PushServer::PushServer(quint16 port, QObject *parent) : QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Push Server"), QWebSocketServer::NonSecureMode, this))
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port))
        qDebug() << "Server listening on port " << port;

    connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &PushServer::onNewConnection);
}

PushServer::~PushServer()
{
    m_pWebSocketServer->close();
    for (auto& [id, socket] : m_clients)
        socket->deleteLater();
}

void PushServer::onNewConnection()
{
    QWebSocket* pSocket = m_pWebSocketServer->nextPendingConnection();

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
