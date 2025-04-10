#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QTableWidget>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTableWidgetItem>
#include <QDebug>
#include <QLineEdit>

class MainWindow : public QWidget {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QWidget(parent) {
        setupUI();
        parseCommandLineArguments();
    }

private:
    void setupUI() {
        importButton = new QPushButton("Import Token File", this);
        exportButton = new QPushButton("Export Token + Vote List", this);
        tableWidget = new QTableWidget(this);
        tokenInput = new QLineEdit(this);
        voteInput = new QLineEdit(this);
        addButton = new QPushButton("Add Token", this);

        importButton->setGeometry(50, 50, 150, 30);
        exportButton->setGeometry(50, 100, 150, 30);
        tableWidget->setGeometry(50, 150, 400, 300);
        tokenInput->setGeometry(50, 470, 150, 30);
        voteInput->setGeometry(220, 470, 150, 30);
        addButton->setGeometry(380, 470, 100, 30);

        QObject::connect(importButton, &QPushButton::clicked, this, &MainWindow::importCSV);
        QObject::connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportCSV);
        QObject::connect(addButton, &QPushButton::clicked, this, &MainWindow::addToken);

        tableWidget->setColumnCount(2);
        tableWidget->setHorizontalHeaderLabels({"Token", "Vote"});
    }

    void importCSV() {
        QString filePath = QFileDialog::getOpenFileName(this, "Import Token File", "", "CSV File (*.csv)");
        if (filePath.isEmpty()) return;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Failed to open file.");
            return;
        }

        QStringList tokens;
        while (!file.atEnd()) {
            QString line = file.readLine().trimmed();
            if (line.isEmpty()) continue;

            QStringList fields = line.split(',');
            if (fields.size() == 2) {
                tokens.append(fields.at(0));  // Store only the token for now
                int row = tableWidget->rowCount();
                tableWidget->insertRow(row);
                tableWidget->setItem(row, 0, new QTableWidgetItem(fields.at(0)));  // Token
                tableWidget->setItem(row, 1, new QTableWidgetItem(fields.at(1)));  // Vote
            }
        }
    }

    void exportCSV() {
        QString filePath = QFileDialog::getSaveFileName(this, "Export Token + Vote List", "", "CSV File (*.csv)");
        if (filePath.isEmpty()) return;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Failed to save the file.");
            return;
        }

        QTextStream out(&file);
        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            QString token = tableWidget->item(row, 0)->text();
            QString vote = tableWidget->item(row, 1)->text();
            out << token << ", " << vote << "\n";  // Write token and vote
        }

        QMessageBox::information(this, "Success", "Token + Vote list exported.");
    }

void addToken() {
    QString token = tokenInput->text().trimmed();
    QString vote = voteInput->text().trimmed();

    if (token.isEmpty() || vote.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter both a token and a vote.");
        return;
    }

    // Check if token already exists in the table
    bool exists = false;
    int rowToUpdate = -1;
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        if (tableWidget->item(row, 0)->text() == token) {
            exists = true;
            rowToUpdate = row;
            break;
        }
    }

    if (exists) {
        // If token exists, overwrite the existing vote
        tableWidget->setItem(rowToUpdate, 1, new QTableWidgetItem(vote));  // Update the vote
        QMessageBox::information(this, "Updated", "Token vote updated.");
    } else {
        // If token doesn't exist, add new row
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        tableWidget->setItem(row, 0, new QTableWidgetItem(token));
        tableWidget->setItem(row, 1, new QTableWidgetItem(vote));
        QMessageBox::information(this, "Added", "Token added with vote.");
    }
}


void addTokenFromCLI(const QString &token, const QString &vote) {
    // Avoid adding duplicates, but allow overwriting
    bool exists = false;
    int rowToUpdate = -1;
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        if (tableWidget->item(row, 0)->text() == token) {
            exists = true;
            rowToUpdate = row;
            break;
        }
    }

    if (exists) {
        // If token exists, overwrite the existing vote
        tableWidget->setItem(rowToUpdate, 1, new QTableWidgetItem(vote));  // Update the vote
        qDebug() << "Token vote updated:" << token << "to" << vote;
    } else {
        // If token doesn't exist, add new row
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        tableWidget->setItem(row, 0, new QTableWidgetItem(token));
        tableWidget->setItem(row, 1, new QTableWidgetItem(vote));
        qDebug() << "Token added:" << token << "with vote:" << vote;
    }
}


    void parseCommandLineArguments() {
        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();

        QCommandLineOption tokenOption("token", "Add a token", "token");
        QCommandLineOption voteOption("vote", "Assign a vote to the token", "vote");

        parser.addOption(tokenOption);
        parser.addOption(voteOption);

        parser.process(*QCoreApplication::instance());

        QString token = parser.value(tokenOption);
        QString vote = parser.value(voteOption);

        if (!token.isEmpty() && !vote.isEmpty()) {
            // If a token and vote are passed as command-line arguments, add them
            addTokenFromCLI(token, vote);
        }
    }


private:
    QPushButton *importButton;
    QPushButton *exportButton;
    QTableWidget *tableWidget;
    QLineEdit *tokenInput;
    QLineEdit *voteInput;
    QPushButton *addButton;
};


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(500, 600);
    window.show();

    return app.exec();
}

#include "main.moc"
