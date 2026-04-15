#include "pushserver.h"
#include "GlobalSettings.h"

#include <QDebug>

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

    UserPushToAllUsers(senderId, message);
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
