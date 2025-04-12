// smtpclient.h

#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>

class SmtpClient : public QObject {
    Q_OBJECT

public:
    SmtpClient(const QString &host, quint16 port, const QString &user, const QString &password);
    bool sendMail(const QString &from, const QStringList &to, const QString &subject, const QString &body);

private:
    QString host, user, password;
    quint16 port;

    bool sendCommand(QSslSocket *socket, const QString &cmd, int expectedCode);
    bool waitForResponse(QSslSocket *socket, int expectedCode);
};

#endif // SMTPCLIENT_H
