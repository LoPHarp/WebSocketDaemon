#ifndef PUSHSERVER_H
#define PUSHSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <unordered_set>
#include <QUrlQuery>

class PushServer : public QObject
{
    Q_OBJECT
public:
    explicit PushServer(quint16 port, QObject *parent = nullptr);
    ~PushServer();

    void ServerPushToUser(int targetUserId, const QString& message);
    void UserPushToAllUsers(int senderId, const QString& message);

private slots:
    void onNewConnection();
    void socketDisconnected();
    void processIncomingMessage(const QString& message);

private:
    QWebSocketServer* m_pWebSocketServer;
    std::unordered_map<int, QWebSocket*> m_clients;
};

#endif // PUSHSERVER_H
