#include <QCoreApplication>
#include <QDebug>
#include "qftp_client.h"
#include "qfile_logger.h"

// Адаптер QFileLogger к интерфейсу QFtpClientLogger
class LoggerAdapter : public QNetUtils::QFtpClientLogger
{
public:
    explicit LoggerAdapter(QNetUtils::QFileLogger* logger) 
        : m_logger(logger) {}

    void log(const QString& message) override {
        if (m_logger) m_logger->log(message);
    }

    void logError(const QString& message) override {
        if (m_logger) m_logger->log("[ERROR] " + message);
    }

private:
    QNetUtils::QFileLogger* m_logger;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QNetUtils::QFileLogger logger;
    logger.init("ftp_upload.log");

    LoggerAdapter adapter(&logger);

    QNetUtils::QFtpClient ftp;
    ftp.setLogger(&adapter);

    QObject::connect(&ftp, &QNetUtils::QFtpClient::connected, [&]() {
        qDebug() << "Connected! Uploading file...";
        ftp.uploadFile("/tmp/test_upload.txt", "uploaded_test.txt");
    });

    QObject::connect(&ftp, &QNetUtils::QFtpClient::uploadProgress,
                     [](qint64 sent, qint64 total) {
        qDebug() << "Progress:" << sent << "/" << total;
    });

    QObject::connect(&ftp, &QNetUtils::QFtpClient::uploadFinished,
                     [&](const QString& path) {
        qDebug() << "File uploaded to:" << path;
        app.quit();
    });

    QObject::connect(&ftp, &QNetUtils::QFtpClient::errorOccurred,
                     [&](const QString& error) {
        qCritical() << "Error:" << error;
        app.quit();
    });

    // Замените на реальные данные
    ftp.connectToServer("127.0.0.1", 2121, "anonymous", "anonymous");

    return app.exec();
}
