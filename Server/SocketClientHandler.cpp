#include "SocketClientHandler.hpp"
#include "ThreadProcessImage.hpp"
#include "ThreadReceiveMessage.hpp"
#include <QDebug>

SocketClientHandler::SocketClientHandler(QTcpSocket* socket, QObject *parent) 
    : QObject(parent), m_socket(socket) {
    
    m_socket->setParent(this);

//    connect(m_socket, &QTcpSocket::readyRead, this, &SocketClientHandler::onReadyRead);
//    connect(m_socket, &QTcpSocket::disconnected, this, &SocketClientHandler::onDisconnected);
}

void SocketClientHandler::onReadyRead() {
    QByteArray data = m_socket->readAll();
}

void SocketClientHandler::onDisconnected() {
    qDebug() << "Client " << m_socket->peerAddress().toString() << "disconnected.";
    emit disconnected(m_socket);
    this->deleteLater(); 
}


SocketClientHandler_Image::SocketClientHandler_Image(QTcpSocket* socket, QObject *parent)
    : SocketClientHandler(socket, parent) {

    connect(m_socket, &QTcpSocket::readyRead, this, &SocketClientHandler_Image::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &SocketClientHandler_Image::onDisconnected);
}

void SocketClientHandler_Image::onReadyRead() {
    QByteArray byteArray = m_socket->readAll();
    socketBufferParser_Image.add_data(byteArray.data(), byteArray.size());
}

void SocketClientHandler_Image::onDisconnected() {
    qDebug() << "8895 Client " << m_socket->peerAddress().toString() << "disconnected.";
    emit disconnected(m_socket);
    this->deleteLater(); 
}

SocketClientHandler_Message::SocketClientHandler_Message(QTcpSocket* socket, QObject *parent)
    : SocketClientHandler(socket, parent) {

    connect(m_socket, &QTcpSocket::readyRead, this, &SocketClientHandler_Message::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &SocketClientHandler_Message::onDisconnected);
}

void SocketClientHandler_Message::onReadyRead() {
    QByteArray byteArray = m_socket->readAll();
    socketBufferParser_Message.add_data(byteArray.data(), byteArray.size());
}

void SocketClientHandler_Message::onDisconnected() {
    qDebug() << "8898 Client " << m_socket->peerAddress().toString() << "disconnected.";
    emit disconnected(m_socket);
    this->deleteLater(); 
}
