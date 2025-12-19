#include "SocketClientHandler.hpp"
#include "ThreadProcessImage.hpp"
#include "ThreadReceiveMessage.hpp"
#include <QDebug>

SocketClientHandler::SocketClientHandler(QTcpSocket* socket, QObject *parent) 
    : QObject(parent), m_socket(socket) {
    
    m_socket->setParent(this);

    connect(m_socket, &QTcpSocket::readyRead, this, &SocketClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &SocketClientHandler::onDisconnected);
}

void SocketClientHandler::onReadyRead() {
    QByteArray data = m_socket->readAll();
    socketBufferParser.add_data(data.data(), data.size());
}

void SocketClientHandler::onDisconnected() {
    qDebug() << "Client " << m_socket->peerAddress().toString() << "disconnected.";
    emit disconnected(m_socket);
    this->deleteLater(); 
}
