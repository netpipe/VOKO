// main.cpp

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include "smtpclient.h"
#include "qdebug.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QString smtpHost = "smtp.example.com";
    quint16 smtpPort = 587;
    QString smtpUser = "your_email@example.com";
    QString smtpPass = "your_password";

    QString from = smtpUser;
    QStringList recipients = {"target@example.com"};

    SmtpClient client(smtpHost, smtpPort, smtpUser, smtpPass);

    QFile file("data.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
   //     qCritical() << "Failed to open CSV file.";
        return 1;
    }

    QTextStream in(&file);
    bool skipHeader = true;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (skipHeader) { skipHeader = false; continue; }

        QStringList parts = line.split(',');
        if (parts.size() < 2) continue;

        QString param1 = parts[0].trimmed();
        QString param2 = parts[1].trimmed();
        QString subject = "CSV Parameters Info";
        QString body = QString("Here are the parameters: %1, %2").arg(param1, param2);

        if (!client.sendMail(from, recipients, subject, body)) {
            qWarning() << "Failed to send email for row:" << line;
        } else {
            qDebug() << "Email sent for:" << line;
        }
    }

    return 0;
}
