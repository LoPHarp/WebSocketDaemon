#ifndef PUSHSERVER_H
#define PUSHSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTimer>
#include <QSslError>
#include <unordered_map>
#include <QUrlQuery>

class PushServer : public QObject
{
    Q_OBJECT
public:
    explicit PushServer(quint16 port, QObject *parent = nullptr);
    ~PushServer();

private slots:
    void onNewConnection();
    void socketDisconnected();
    void processIncomingMessage(const QString& message);
    void sendPings();
    void onSslErrors(const QList<QSslError>& errors);

private:
    void ServerPushToUser(int targetUserId, const QString& message);
    void UserPushToAllUsers(int senderId, const QString& message);

    QWebSocketServer* m_pWebSocketServer;
    std::unordered_map<int, QWebSocket*> m_clients;
    QTimer* m_pingTimer;
};

#endif // PUSHSERVER_H
