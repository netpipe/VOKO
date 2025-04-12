// smtpclient.cpp

#include "smtpclient.h"
#include <QDebug>

SmtpClient::SmtpClient(const QString &host, quint16 port, const QString &user, const QString &password)
    : host(host), port(port), user(user), password(password) {}

bool SmtpClient::sendMail(const QString &from, const QStringList &to, const QString &subject, const QString &body) {
    QSslSocket socket;
    socket.connectToHostEncrypted(host, port);
    if (!socket.waitForEncrypted(5000)) return false;

    if (!waitForResponse(&socket, 220)) return false;
    if (!sendCommand(&socket, "EHLO localhost\r\n", 250)) return false;
    if (!sendCommand(&socket, "AUTH LOGIN\r\n", 334)) return false;
    if (!sendCommand(&socket, QByteArray().append(user.toUtf8().toBase64()).append("\r\n"), 334)) return false;
    if (!sendCommand(&socket, QByteArray().append(password.toUtf8().toBase64()).append("\r\n"), 235)) return false;

    if (!sendCommand(&socket, "MAIL FROM:<" + from + ">\r\n", 250)) return false;

    for (const QString &recipient : to) {
        if (!sendCommand(&socket, "RCPT TO:<" + recipient + ">\r\n", 250)) return false;
    }

    if (!sendCommand(&socket, "DATA\r\n", 354)) return false;

    QString message = QString("Subject: %1\r\nTo: %2\r\nFrom: %3\r\n\r\n%4\r\n.\r\n")
                          .arg(subject)
                          .arg(to.join(", "))
                          .arg(from)
                          .arg(body);

    socket.write(message.toUtf8());
    socket.flush();
    if (!waitForResponse(&socket, 250)) return false;

    sendCommand(&socket, "QUIT\r\n", 221);
    socket.disconnectFromHost();
    return true;
}

bool SmtpClient::sendCommand(QSslSocket *socket, const QString &cmd, int expectedCode) {
    socket->write(cmd.toUtf8());
    socket->flush();
    return waitForResponse(socket, expectedCode);
}

bool SmtpClient::waitForResponse(QSslSocket *socket, int expectedCode) {
    if (!socket->waitForReadyRead(5000)) return false;
    while (socket->canReadLine()) {
        QString line = socket->readLine();
        if (line.startsWith(QString::number(expectedCode))) return true;
    }
    return false;
}
