#ifndef SOCKETCLIENTHANDLER_HPP
#define SOCKETCLIENTHANDLER_HPP

#include <QObject>
#include <QTcpSocket>
#include "SocketBufferParser.hpp"

class SocketClientHandler : public QObject {
    Q_OBJECT
public:
    explicit SocketClientHandler(QTcpSocket* socket, QObject *parent = nullptr);
    SocketBufferParser socketBufferParser;

signals:
    void disconnected(QTcpSocket* socket);
protected slots:
    void onReadyRead();
    void onDisconnected();

protected:
    QTcpSocket* m_socket;
};

#endif