#include "qfile_logger.h"
#include <QFileInfo>
#include <QDir>

namespace QNetUtils {

QFileLogger::QFileLogger(QObject *parent)
    : QObject(parent)
{
}

QFileLogger::~QFileLogger()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }
}

bool QFileLogger::init(const QString& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    QFileInfo info(filePath);
    QDir dir = info.absoluteDir();
    
    // Создаём директорию, если её нет
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString baseName = info.completeBaseName();
    QString suffix = info.suffix();
    QString dirPath = info.absolutePath();

    QString timestamp = QDateTime::currentDateTime()
                            .toString("yyyy-MM-dd_HH-mm-ss");

    QString newFileName = QString("%1/%2_%3.%4")
                              .arg(dirPath, baseName, timestamp, suffix);

    m_file.setFileName(newFileName);
    return m_file.open(QIODevice::Append | QIODevice::Text);
}

void QFileLogger::log(const QString& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    QString timestamp = QDateTime::currentDateTime()
                            .toString("yyyy-MM-dd HH:mm:ss");

    QString fullMessage = QString("[%1] %2").arg(timestamp, message);

    if (m_file.isOpen()) {
        QTextStream out(&m_file);
        out << fullMessage << "\n";
        out.flush();
    }

    emit newLogMessage(fullMessage);
}

bool QFileLogger::isOpen() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_file.isOpen();
}

} // namespace QNetUtils
