﻿#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QDateTime>
#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QRandomGenerator>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QCalendarWidget>
#include <QSettings>

QLabel *tokensleftlbl;
QLineEdit *tokenstxt;
QCalendarWidget *test2;
QSettings *settings;
QLineEdit *hours;
QLineEdit *tokenvaltxt;

// might export votes with tokens to make the vote files easier to reassemble ?

//returned value of tokens might be handy for a voting system or to show how much change it has.

int getTokensLeft() {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM valid_tokens WHERE redeemed = 0");

    if (!query.exec()) {
        qDebug() << "Failed to execute query:" << query.lastError().text();
        return -1; // Return -1 to indicate an error
    }

    if (query.next()) {
        return query.value(0).toInt(); // Return the number of remaining tokens
    }

    return 0; // In case no tokens are left
}

void tokensleft()
{
   // qDebug() << "test";
    tokensleftlbl->setText(QString::number(getTokensLeft()) );
}

QString generateRandomToken(int length = 12) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString token;
    for (int i = 0; i < length; ++i) {
        int index = QRandomGenerator::global()->bounded(chars.length());
        token += chars.at(index);
    }
    return token;
}

void bruteForceTokenPool2(int total = 100000) {
    QSqlQuery clear;
    clear.exec("DELETE FROM all_tokens");

    QSqlQuery insert;
    insert.prepare("INSERT OR IGNORE INTO all_tokens (token) VALUES (:token)");

    int inserted = 0;
    while (inserted < total) {
        QString token = generateRandomToken(12);
        insert.bindValue(":token", token);
        insert.exec();

        if (insert.numRowsAffected() > 0) {
            inserted++;
        }
    }
}

void selectValidTokens(int count) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(0, "Question", "select new token pool ?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
    QSqlQuery q;
    q.exec("DELETE FROM valid_tokens");
    QSqlQuery insert;
    insert.prepare("INSERT INTO valid_tokens (token) SELECT token FROM all_tokens ORDER BY RANDOM() LIMIT :count");
    insert.bindValue(":count", count);
    insert.exec();

    tokensleft();
    }
}

QString validateTokenRedemption(const QString &token,int vote) {
    QSqlQuery check;
    check.prepare("SELECT 1 FROM all_tokens WHERE token = :token");
    check.bindValue(":token", token);
    if (!check.exec() || !check.next()) {
        return "Invalid Token: Not recognized (forged).";
    }

    QSqlQuery valid;
    valid.prepare("SELECT redeemed FROM valid_tokens WHERE token = :token");
    valid.bindValue(":token", token);
    if (!valid.exec() || !valid.next()) {
        return "Invalid Token: Not part of active set.";
    }

    if (valid.value(0).toBool() == 0) {
        return "Token already redeemed.";
    }

    QSqlQuery update;
    update.prepare("UPDATE valid_tokens SET redeemed = 0 , returned_value = :value, returned_at = :date WHERE token = :token");
    update.bindValue(":vote",1);
    update.bindValue(":date",QDateTime::currentDateTime().toString(Qt::ISODate));
    update.bindValue(":token", token);
    update.exec();
    return "Token successfully redeemed.";

}

int hoursUntilDate(const QDate &targetDate) {
    QDate today = QDate::currentDate();
    int daysDiff = today.daysTo(targetDate);
    if (daysDiff == 0 && hours->text() != "" ){

    return hours->text().toInt();
    } else {
            hours->setText(QString::number(daysDiff * 24));
    }
   // return daysDiff * 24;
}


QString generateTokenFile(QString count2, int hoursToExpire) {
   settings->setValue("year", test2->selectedDate().year());
    settings->setValue("month", test2->selectedDate().month());
     settings->setValue("day", test2->selectedDate().day());
           //  qDebug() << hoursUntil(datetest);
   int count = count2.toInt();
   int counted=0;

    if (count != 0 && count2 != "") {
    QSqlQuery query;
    query.prepare("SELECT token FROM valid_tokens WHERE redeemed = 0 LIMIT :count"); //random ?
    query.bindValue(":count", count);
    query.exec();

    QStringList tokens;
    while (query.next()) {
        tokens << query.value(0).toString();
        counted = counted + 1;
    }

    // check if enough count
    qDebug() << counted << count << endl;
    if ( counted == count ) {
    // Mark selected tokens as redeemed immediately


    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString expiry = QDateTime::currentDateTime().addSecs(hoursToExpire * 3600).toString(Qt::ISODate);

    QStringList lines;
    lines << "# IOU Export File";
    lines << "Timestamp: " + timestamp;
    lines << "Expires: " + expiry;
    lines << "Tokens:";
    //regional tokens for verification purposes

    for (const QString &t : tokens) lines << t;

    QString rawData = lines.join("\n");
    QByteArray md5 = QCryptographicHash::hash(rawData.toUtf8(), QCryptographicHash::Md5).toHex();
    lines << "MD5: " + QString(md5);

    // Create file name based on timestamp for backup and export
    #ifdef __APPLE__
    QString filePath = QFileDialog::getSaveFileName(nullptr, "Export Token File", "/Applications/VOKO.app/Contents/MacOS/files/", "Token File (*.iou)");
     #else
    QString filePath = QFileDialog::getSaveFileName(nullptr, "Export Token File", "", "Token File (*.iou)");
    #endif
    if (!filePath.isEmpty()) {
        QSqlQuery update;
        update.prepare("UPDATE valid_tokens SET redeemed = 1 WHERE token = :token");

        for (const QString &token : tokens) {
            update.bindValue(":token", token);
            update.exec();
        }

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << lines.join("\n");
            file.close();

            // Backup file with timestamped name
            QString backupPath = QFileInfo(filePath).absolutePath() + "/tokens_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".bak";
            QFile::copy(filePath, backupPath);  // Create backup copy

            // Store backup file details (including checksum) in the database
            QSqlQuery insertFile;
            insertFile.prepare("INSERT INTO transaction_files (file_path, backup_path, md5_checksum, expiry_time) VALUES (:file_path, :backup_path, :md5_checksum, :expiry_time)");
            insertFile.bindValue(":file_path", filePath);
            insertFile.bindValue(":backup_path", backupPath);
            insertFile.bindValue(":md5_checksum", QString(md5));
            insertFile.bindValue(":expiry_time", expiry);
            insertFile.exec();

            tokensleft();
            tokenstxt->setText("");

            return QString("Exported %1 tokens to %2").arg(tokens.size()).arg(filePath);
        }
    }
    }
    }
    return "Export cancelled or failed.";
}

void restoreExpiredTokensFromBackup() {
    QSqlQuery query;
    query.prepare("SELECT backup_path, md5_checksum, expiry_time FROM transaction_files WHERE expiry_time < :current_time");
    query.bindValue(":current_time", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        qDebug() << "Failed to retrieve expired transaction files.";
        return;
    }
//qDebug() << "test";
    while (query.next()) {
        QString backupPath = query.value(0).toString();
        QString storedMD5 = query.value(1).toString();
        QDateTime expiry = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);

        if (expiry < QDateTime::currentDateTime()) {
            // Restore tokens only if the MD5 checksum matches
            QFile backupFile(backupPath);
            if (backupFile.exists() && backupFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QByteArray backupData = backupFile.readAll();

                QString backupText = QString::fromUtf8(backupData);
                QStringList lines2 = backupText.split('\n');


                int md5Index = lines2.lastIndexOf(QRegExp("^MD5: .*"));
               if (md5Index == -1) qDebug() << "Invalid file format: No MD5 found.";

                // Only join lines up to (but not including) the MD5 line
                QString rawData = lines2.mid(0, md5Index).join("\n");


                QByteArray calculatedMD52 = QCryptographicHash::hash(rawData.toUtf8(), QCryptographicHash::Md5).toHex();
        //qDebug() << storedMD5 << QString(calculatedMD52) ;
               // if (storedMD5 == QString(calculatedMD52)) {
                    QTextStream in(&backupFile);

                    int tokensStart = -1;

                    for (int i = 0; i < lines2.size(); ++i) {
                        QString line = lines2[i].trimmed();
                        if (line.startsWith("Tokens:")) {
                            tokensStart = i + 1;
                        }
                    }

                        QStringList tokensToRestore = lines2.mid(tokensStart, md5Index - tokensStart);

                    for (const QString &token : tokensToRestore) {
                        // Mark tokens as unredeemed
                        QSqlQuery q;
                        q.prepare("UPDATE valid_tokens SET redeemed = 0 WHERE token = :token");
                        q.bindValue(":token", token);
                        q.exec();
                     //   qDebug() << token;
                    }
                    qDebug() << "restored tokens";
                    backupFile.close();
                    backupFile.remove();
               // } else {
                //    qDebug() << "MD5 checksum mismatch for backup file: " << backupPath;
                //}
            }
        }
    }
}

void bruteForceTokenPool(int total = 100000, int batchSize = 1000) {
    //sql automatically removes doubles
    QSqlQuery clear;
    clear.exec("DELETE FROM all_tokens");

    QSqlDatabase::database().transaction();

    QSet<QString> uniqueTokens;
    while (uniqueTokens.size() < total) {
        uniqueTokens.insert(generateRandomToken(25));
    }

    QStringList tokens = uniqueTokens.values();
    QSqlQuery insert;
    for (int i = 0; i < tokens.size(); i += batchSize) {
        QStringList batch;
        for (int j = i; j < i + batchSize && j < tokens.size(); ++j) {
            batch << QString("('%1')").arg(tokens[j]);
        }
        QString sql = "INSERT OR IGNORE INTO all_tokens (token) VALUES " + batch.join(", ");
        insert.exec(sql);
    }

    QSqlDatabase::database().commit();
}

void removeTransactionFileByMD5(const QString &md5) {
    QSqlQuery query;
    query.prepare("DELETE FROM transaction_files WHERE md5 = :md5");
    query.bindValue(":md5", md5);
    if (!query.exec()) {
        qDebug() << "Failed to remove transaction file entry:" << query.lastError().text();
    } else {
        qDebug() << "Removed transaction file entry for MD5:" << md5;
    }
}

QString importTokenFile(QString file2) {
    QString filePath;
    if (file2.isEmpty()){
     filePath = QFileDialog::getOpenFileName(nullptr, "Import Token File", "", "Token File (*.iou)");
    if (filePath.isEmpty()) return "Import cancelled.";
    }else{
        filePath = file2;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "Failed to open file.";

    QStringList lines;
    while (!file.atEnd()) lines << file.readLine().trimmed();

    // Extract and validate the MD5 checksum
    int md5Index = lines.lastIndexOf(QRegExp("^MD5: .*"));
    if (md5Index == -1) return "Invalid file format: No MD5 found.";

    QString originalMD5 = lines[md5Index].mid(5).trimmed();
    QString rawData = lines.mid(0, md5Index).join("\n");
    QByteArray calculatedMD5 = QCryptographicHash::hash(rawData.toUtf8(), QCryptographicHash::Md5).toHex();
    if (originalMD5 != calculatedMD5) return "MD5 mismatch: File integrity compromised.";
    qDebug() << "md5 sum" << calculatedMD5;

    // Check expiry
    QString expiryLine = lines.filter(QRegExp("^Expires:")).value(0);
    QDateTime expiry = QDateTime::fromString(expiryLine.mid(8).trimmed(), Qt::ISODate);
    if (expiry.isValid() && expiry < QDateTime::currentDateTime()) {
       // return "Token file expired on " + expiry.toString();
    }

    // Find and restore tokens from the file
    int tokenStart = lines.indexOf("Tokens:") + 1;
    int tokenEnd = md5Index;
    QStringList importedTokens = lines.mid(tokenStart, tokenEnd - tokenStart);

    // Loop through the tokens and update their status in the database
    int redeemed = 0;
    for (const QString &token : importedTokens) {
        QSqlQuery q;
       // q.prepare("SELECT valid_tokens WHERE token = :token AND redeemed = 1");
          q.prepare("SELECT redeemed FROM valid_tokens WHERE token = :token");
        q.bindValue(":token", token);
        q.exec();
        if (q.next()){
            redeemed++;}

    }
qDebug() << redeemed;



    for (const QString &token : importedTokens) {
        QSqlQuery q;
        q.prepare("UPDATE valid_tokens SET redeemed = 1 WHERE token = :token AND redeemed = 0");
        q.bindValue(":token", token);
     //   q.exec();
        if (q.numRowsAffected() > 0) redeemed++;
    }


        if (importedTokens.size() == redeemed){
            redeemed=0;
    // Check for backup files (expired ones), restore if needed
    QSqlQuery queryBackup;
  //  queryBackup.prepare("SELECT backup_path, md5_checksum, expiry_time FROM transaction_files WHERE expiry_time < :current_time");


        queryBackup.prepare("SELECT backup_path, md5_checksum, expiry_time FROM transaction_files");
    queryBackup.bindValue(":current_time", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!queryBackup.exec()) {
        qDebug() << "Failed to retrieve expired transaction files.";
        return "Error retrieving expired transaction files.";
    }
 QString backupPath;
    while (queryBackup.next()) {
        backupPath = queryBackup.value(0).toString();
        QString storedMD5 = queryBackup.value(1).toString();
        QDateTime expiry = QDateTime::fromString(queryBackup.value(2).toString(), Qt::ISODate);

        if (expiry > QDateTime::currentDateTime()) {
            QFile backupFile(backupPath);

            if (backupFile.exists() && backupFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
             //    if (backupFile.exists() ) {
            // qDebug() << "backup exists2";
                QByteArray backupData = backupFile.readAll();
              //  QByteArray calculatedMD5 = QCryptographicHash::hash(backupData, QCryptographicHash::Md5).toHex();

                QString backupText = QString::fromUtf8(backupData);
                QStringList lines2 = backupText.split('\n');


                int md5Index = lines2.lastIndexOf(QRegExp("^MD5: .*"));
                if (md5Index == -1) return "Invalid file format: No MD5 found.";

                // Only join lines up to (but not including) the MD5 line
                QString rawData = lines2.mid(0, md5Index).join("\n");

                QByteArray calculatedMD52 = QCryptographicHash::hash(rawData.toUtf8(), QCryptographicHash::Md5).toHex();


                // Validate MD5 checksum
                if (originalMD5 == QString(calculatedMD52)) { //todo
              //      if (1){
             //  if (storedMD5 == QString(originalMD5)) {
               ///     qDebug() << "matches " << backupPath;
                    qDebug() << "testing " << QString(calculatedMD52) ;
                   // QTextStream in(&backupFile);
                   // QStringList backupLines;
                   // while (!in.atEnd()) {
                   //     backupLines << in.readLine().trimmed();
                   // }

                    int md5Index = -1;
                    int tokensStart = -1;

                    for (int i = 0; i < lines2.size(); ++i) {
                        QString line = lines2[i].trimmed();
                        if (line.startsWith("Tokens:")) {
                            tokensStart = i + 1;
                        } else if (line.startsWith("MD5:")) {
                            md5Index = i;
                            break;
                        }
                    }

                  //  if (tokensStart == -1 || md5Index == -1 || tokensStart >= md5Index) {
                 //       qDebug() << "Invalid file: couldn't detect token block.";
                   // } else {
                        QStringList backupTokens = lines2.mid(tokensStart, md5Index - tokensStart);
                  //      qDebug() << "Parsed tokens:" << backupTokens;
                  //      qDebug() << "Token count:" << backupTokens.count();
                  //  }

                  //  qDebug() << backupTokens[1].toLatin1();
 //qDebug() << backupTokens.count() ;
                    // Restore tokens from the backup file


                    for (const QString &token : backupTokens) {

                    //     qDebug() << validateTokenRedemption(token);
                      if ( validateTokenRedemption(token,tokenvaltxt->text().toInt()) == "Token successfully redeemed." ){
                    //    QSqlQuery restoreQuery;
                   //     restoreQuery.prepare("UPDATE valid_tokens SET redeemed = 0 WHERE token = :token");
                    //   restoreQuery.bindValue(":token", token);
                     //   restoreQuery.exec();
                     //   qDebug() << token ;
                         // qDebug() << "redeem";
                       redeemed = redeemed + 1;

                       QSqlQuery update;
                       update.prepare(R"(
                           UPDATE valid_tokens
                           SET redeemed = 0,
                               returned_at = :returnedAt,
                               returned_value = :value
                           WHERE token = :token AND redeemed = 0
                       )");
                       update.bindValue(":returnedAt", QDateTime::currentDateTime().toString(Qt::ISODate));
                       update.bindValue(":value", 1.0); // or whatever portion is returned
                       update.bindValue(":token", token);
                       update.exec();

                          }
                    }

                    backupFile.close();
                    removeTransactionFileByMD5(originalMD5);

                } else {
                  //  qDebug() << "MD5 checksum mismatch for backup file: " << backupPath;
                }
            }
        }
    }


        }else
        {
                QMessageBox::information(0, "Alert", "tokenfile not valid dont pay ?",QMessageBox::Ok);

        }
    if (importedTokens.size() != redeemed){
    QMessageBox::information(0, "Alert", "tokenfile not valid dont pay ?",QMessageBox::Ok);
  //  qDebug() << backupPath;
    }
        // if they match do cashout otherwise message invalid
    return QString("Imported %1 tokens. %2 were valid and redeemed.").arg(importedTokens.size()).arg(redeemed);
}


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("Secure IOU Token System");
    auto *layout = new QVBoxLayout(&window);
        QLineEdit *tokengen =  new QLineEdit;


        QCommandLineParser parser;
        parser.setApplicationDescription("Secure IOU Token System");
        parser.addHelpOption();
        parser.addVersionOption();

        // Define options
        QCommandLineOption generateAllOpt("generate-all", "Generate all tokens");
        QCommandLineOption selectValidOpt("select-valid", "Select valid tokens");
        QCommandLineOption redeemOpt("redeem", "Redeem a token", "token");
        QCommandLineOption exportOpt("export", "Export N tokens", "count");
        QCommandLineOption eTimeOpt("etime", "Export time", "count");
        QCommandLineOption voteOpt("vote", "Vote Number", "count");
        QCommandLineOption importOpt("import", "Import tokens from file", "file");
        QCommandLineOption headlessOpt("headless", "Run without GUI");

        parser.addOption(generateAllOpt);
        parser.addOption(selectValidOpt);
        parser.addOption(redeemOpt);
        parser.addOption(exportOpt);
                parser.addOption(eTimeOpt);
                parser.addOption(voteOpt);
        parser.addOption(importOpt);
        parser.addOption(headlessOpt);

        parser.process(app);


    //load config file with last used database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    #ifdef __APPLE__
    db.setDatabaseName("/Applications/VOKO.app/Contents/MacOS/tokens.db");
    #else
       db.setDatabaseName("tokens.db");
    #endif
    db.open();

    QSqlQuery init;
    init.exec("CREATE TABLE IF NOT EXISTS all_tokens (token TEXT PRIMARY KEY, token_expiry DATETIME)");
    init.exec("CREATE TABLE IF NOT EXISTS valid_tokens (token TEXT PRIMARY KEY, redeemed INTEGER DEFAULT 0, token_expiry DATETIME, FOREIGN KEY(token) REFERENCES all_tokens(token))");
    init.exec("CREATE TABLE IF NOT EXISTS transaction_files (file_path TEXT, backup_path TEXT, md5_checksum TEXT, expiry_time DATETIME)");
    init.exec("ALTER TABLE valid_tokens ADD COLUMN returned_at TEXT");      //   -- ISO datetime string
    init.exec("ALTER TABLE valid_tokens ADD COLUMN returned_value REAL");  //    -- Decimal or fractional value


    // Command-line actions
    if (parser.isSet(generateAllOpt)) {
        bruteForceTokenPool(tokengen->text().toInt()*1.5,1000);
        qDebug() << "Generated all tokens.";    }

    if (parser.isSet(selectValidOpt)) {
        selectValidTokens(tokengen->text().toInt());
        qDebug() << "Selected valid tokens.";    }

    if (parser.isSet(redeemOpt) && !parser.isSet(voteOpt) ) {
        QString token = parser.value(redeemOpt);
        qDebug() << validateTokenRedemption(token,1);    }

    if (parser.isSet(exportOpt) && parser.isSet(eTimeOpt) ) {
        qDebug() << generateTokenFile(parser.value(exportOpt),parser.value(eTimeOpt).toInt());    }

    if (parser.isSet(redeemOpt) && parser.isSet(voteOpt) ) {
        QString token = parser.value(redeemOpt);
        QString voteOpt2 = parser.value(voteOpt);
        qDebug() << validateTokenRedemption(token,voteOpt2.toInt());
    }

    if (parser.isSet(importOpt)) {
        QString filePath = parser.value(importOpt);
        // Temporarily override QFileDialog:
        QFile file(filePath);
        if (file.exists()) {
         //   QFileDialog::setFileMode(QFileDialog::ExistingFile);
        //    QFileDialog::setOption(QFileDialog::DontUseNativeDialog);
         //   QFileDialog::selectFile(filePath);
            qDebug() << importTokenFile(filePath);
        } else {
            qDebug() << "Import file not found:" << filePath;
        }
    }

    if (parser.isSet(headlessOpt)) {        return 0;  // Skip GUI
    }


    // Reset expired tokens at app startup
    restoreExpiredTokensFromBackup();

    auto *output = new QPlainTextEdit;
    output->setReadOnly(true);
    auto *tokenInput = new QLineEdit;
    tokenInput->setPlaceholderText("Enter token to redeem...");

    auto *redeemBtn = new QPushButton("Redeem Token");
    auto *genAllBtn = new QPushButton("Generate All Tokens");
    auto *genValidBtn = new QPushButton("Select Valid Tokens");


    test2 = new QCalendarWidget;
    tokenstxt =  new QLineEdit;

    tokenvaltxt =  new QLineEdit;
    tokenvaltxt->setText("1");
    hours =  new QLineEdit;
    hours->setText("24");

    QSplitter *splitter = new QSplitter;
    QSplitter *splitter2 = new QSplitter;
    QSplitter *timesplit = new QSplitter;
    QSplitter *timesplit2 = new QSplitter;
    QLabel *tokenslbl = new QLabel;
    QLabel *hourslbl = new QLabel;
    QLabel *votelbl = new QLabel;
    hourslbl->setText("Valid For (hours)");
    votelbl->setText("Vote Number");

    tokensleftlbl = new QLabel;
    tokenslbl->setText("tokens");
    tokengen->setText("10000");

    QLabel *ee = new QLabel;
    ee->setText("Remaining");


    auto *exportBtn = new QPushButton("Export Tokens to File");
    auto *importBtn = new QPushButton("Import & Redeem Token File");


    settings = new QSettings("NP", "VOKO", 0);
    QString dates = settings->value("dates", "").toString();
    QString day = settings->value("day", "").toString();
    QString month = settings->value("month", "").toString();
        QString year = settings->value("year", "").toString();

        QDate datetest = test2->selectedDate();
    if (day != ""){
      //  datetest.fromString(dates.toLatin1() );
        datetest.setDate(year.toInt(),month.toInt(),day.toInt());
        test2->setSelectedDate(datetest);
  //  settings->setValue("year", test2->selectedDate().year());
}

    timesplit->addWidget(hourslbl);
     timesplit->addWidget(hours);

    layout->addWidget(tokenInput);
    layout->addWidget(redeemBtn);
    splitter->addWidget(tokengen);
    splitter->addWidget(genAllBtn);
    splitter->addWidget(genValidBtn);
    layout->addWidget(splitter);
    splitter2->addWidget(tokenslbl);
    splitter2->addWidget(tokenstxt);

    splitter2->addWidget(votelbl);
    splitter2->addWidget(tokenvaltxt);

    splitter2->addWidget(ee);
        splitter2->addWidget(tokensleftlbl);
             layout->addWidget(timesplit);
        //     layout->addWidget(timesplit2);
             layout->addWidget(test2);


      layout->addWidget(splitter2);
    layout->addWidget(exportBtn);
    layout->addWidget(importBtn);
    layout->addWidget(output);

    tokensleft();

    //accounting for expected payout value
                       // QSqlQuery q("SELECT token, returned_at, returned_value FROM valid_tokens WHERE redeemed = 1");
                       // while (q.next()) {
                       //     QString token = q.value(0).toString();
                       //     QString returnedAt = q.value(1).toString();
                       //     double returnedValue = q.value(2).toDouble();
                         //   qDebug() << "Token:" << token << "Returned on:" << returnedAt << "Value:" << returnedValue;

    QObject::connect(test2, &QCalendarWidget::clicked, [](const QDate & date) {
          qDebug() << hoursUntilDate(date);});

   QTimer mytimer;
   QObject::connect(&mytimer,&QTimer::timeout,tokensleft);
   mytimer.start(5000);

      QTimer mytimer2;
   QObject::connect(&mytimer2,&QTimer::timeout,restoreExpiredTokensFromBackup);
   mytimer2.start(500000);

    QObject::connect(redeemBtn, &QPushButton::clicked, [&]() {
        QString result = validateTokenRedemption(tokenInput->text().trimmed(),tokenvaltxt->text().toInt());
        output->appendPlainText(result);
    });
    QObject::connect(genAllBtn, &QPushButton::clicked, [&]() {
        bruteForceTokenPool(tokengen->text().toInt()*1.5,1000);
        output->appendPlainText("Generated all tokens.");
    });
    QObject::connect(genValidBtn, &QPushButton::clicked, [&]() {
        selectValidTokens(tokengen->text().toInt());
        output->appendPlainText("Selected valid tokens.");
    });
    QObject::connect(exportBtn, &QPushButton::clicked, [&]() {
        output->appendPlainText(generateTokenFile(tokenstxt->text(),hours->text().toInt()));
    });
    QObject::connect(importBtn, &QPushButton::clicked, [&]() {
        output->appendPlainText(importTokenFile(""));
    });

    window.resize(500, 500);
    window.show();
    return app.exec();
}

