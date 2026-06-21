#include <QCoreApplication>
#include <QDebug>
#include "qfile_logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QNetUtils::QFileLogger logger;
    
    // Подписываемся на сигнал для вывода в консоль
    QObject::connect(&logger, &QNetUtils::QFileLogger::newLogMessage,
                     [](const QString& msg) {
        qDebug().noquote() << msg;
    });

    if (!logger.init("demo.log")) {
        qCritical() << "Failed to initialize logger!";
        return 1;
    }

    logger.log("Application started");
    logger.log("Processing data...");
    logger.log("Application finished");

    qDebug() << "Check demo.log file for results";
    return 0;
}
