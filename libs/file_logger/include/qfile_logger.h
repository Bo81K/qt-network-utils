#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <mutex>

namespace QNetUtils {

/*!
 * \brief The QFileLogger class Thread-safe file logger for Qt applications
 *
 * Provides timestamped message logging to a text file with automatic
 * unique filename generation. Emits signals for real-time UI integration.
 */
class QFileLogger : public QObject
{
    Q_OBJECT

public:
    explicit QFileLogger(QObject *parent = nullptr);
    ~QFileLogger() override;

    /*!
     * \brief Initialize logger and create log file
     *
     * Generates unique filename by appending timestamp to base name
     * (e.g., app.log -> app_2023-10-27_14-30-00.log)
     *
     * \param filePath Base path to log file
     * \return true if file opened successfully, false on access error
     */
    bool init(const QString& filePath);

    /*!
     * \brief Log a message
     *
     * Thread-safe method that adds timestamp, writes to file,
     * and emits newLogMessage signal for subscribers.
     *
     * \param message Message text to log
     */
    void log(const QString& message);

    /*!
     * \brief Check if logger is initialized and file is open
     */
    bool isOpen() const;

signals:
    /*!
     * \brief Emitted when new log message is written
     *
     * Used for real-time log display in UI (e.g., QTextEdit).
     *
     * \param message Complete message with timestamp
     */
    void newLogMessage(const QString& message);

private:
    QFile m_file;
    mutable std::mutex m_mutex;
};

} // namespace QNetUtils
