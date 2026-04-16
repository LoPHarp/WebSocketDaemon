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
    void shutdown();
    ~PushServer();

private slots:
    void sendPings();
    void onSslErrors(const QList<QSslError>& errors);
    void onNewConnection();
    void processIncomingMessage(const QString& message);
    void socketDisconnected();


private:
    void UserPushToAllUsers(const QString& senderId, const QString& message);
    void ServerPushToUserOrRole(const QString& targetIdOrRole, const QString& message);

    QWebSocketServer* m_pWebSocketServer;
    std::unordered_map<QString, QList<QWebSocket*>> m_clients;
    QTimer* m_pingTimer;
};

#endif // PUSHSERVER_H
