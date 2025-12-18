#ifndef SOCKETCLIENTHANDLER_HPP
#define SOCKETCLIENTHANDLER_HPP

#include <QObject>
#include <QTcpSocket>
#include "SocketBufferParser.hpp"

class SocketClientHandler : public QObject {
    Q_OBJECT
public:
    explicit SocketClientHandler(QTcpSocket* socket, QObject *parent = nullptr);

signals:
    void disconnected(QTcpSocket* socket);
protected slots:
    void onReadyRead();
    void onDisconnected();

protected:
    QTcpSocket* m_socket;
};


class SocketClientHandler_Image : public SocketClientHandler {
    Q_OBJECT
public:
    SocketClientHandler_Image(QTcpSocket* socket, QObject *parent = nullptr);
    SocketBufferParser_Image socketBufferParser_Image;
private:
    void onReadyRead();
    void onDisconnected();
};

class SocketClientHandler_Message : public SocketClientHandler {
    Q_OBJECT
public:
    SocketClientHandler_Message(QTcpSocket* socket, QObject *parent = nullptr);
    SocketBufferParser_Message socketBufferParser_Message;
private:
    void onReadyRead();
    void onDisconnected();
};

#endif