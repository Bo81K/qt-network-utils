#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QTimer>
#include <QPointer>

namespace QNetUtils {

/*!
 * \brief Abstract logger interface for QFtpClient
 *
 * Implement this interface to provide custom logging.
 * Example: integrate with QFileLogger or your own logging system.
 */
class QFtpClientLogger
{
public:
    virtual ~QFtpClientLogger() = default;
    virtual void log(const QString& message) = 0;
    virtual void logError(const QString& message) = 0;
};

/*!
 * \brief Asynchronous FTP client based on State Machine pattern
 *
 * Implements FTP protocol with separate control and data sockets.
 * Supports passive mode (PASV), file upload with progress tracking.
 * All network operations are non-blocking and managed via internal state machine.
 */
class QFtpClient : public QObject
{
    Q_OBJECT

public:
    explicit QFtpClient(QObject *parent = nullptr);
    ~QFtpClient() override;

    /*!
     * \brief Initiate connection to FTP server
     * \param host IP address or hostname
     * \param port FTP server port (default: 21)
     * \param user Username
     * \param pass Password
     */
    void connectToServer(const QString& host, quint16 port = 21,
                         const QString& user = "",
                         const QString& pass = "");

    /*!
     * \brief Disconnect from server
     *
     * Sends QUIT command, closes sockets, and resets state.
     */
    void disconnectFromServer();

    /*!
     * \brief Check authorization status
     * \return true if client is logged in (State::LoggedIn)
     */
    bool isConnected() const { return m_state == State::LoggedIn; }

    /*!
     * \brief Upload local file to server
     *
     * Automatically switches to passive mode (PASV) and opens data socket.
     *
     * \param localPath Path to local file
     * \param remotePath Target path on FTP server
     */
    void uploadFile(const QString& localPath, const QString& remotePath);

    /*!
     * \brief Change current directory on server
     * \param path Target directory path
     */
    void changeDirectory(const QString& path);

    /*!
     * \brief Get current remote directory
     */
    QString currentRemotePath() const { return m_currentRemotePath; }

    /*!
     * \brief Set logger for debug messages
     * \param logger Pointer to logger implementation (can be nullptr)
     */
    void setLogger(QFtpClientLogger* logger) { m_logger = logger; }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    void uploadProgress(qint64 sent, qint64 total);
    void uploadFinished(const QString& remotePath);

private slots:
    void onControlReadyRead();
    void onDataReadyRead();
    void onControlConnected();
    void onControlError(QAbstractSocket::SocketError);
    void onSocketDisconnected();
    void onTimeout();
    void onDataSocketDisconnected();
    void onDataBytesWritten(qint64 bytes);

private:
    enum class State {
        Idle,
        Connecting,
        WaitingUser,
        WaitingPass,
        LoggingIn,
        LoggedIn,
        WaitingPasv,
        WaitingTransfer,
        Transferring,
        Disconnected,
        Closing
    };

    void sendCommand(const QString& cmd);
    void processReply(const QString& reply);
    void openDataConnection(const QString& host, quint16 port);
    void resetState();
    void abortCurrentOperation();
    void finalizeTransfer();
    void startUpload();
    void sendDataChunk();
    void log(const QString& message);
    void logError(const QString& message);

    QTcpSocket* m_controlSocket;
    QTcpSocket* m_dataSocket;
    QTimer* m_timeout;
    QFile* m_file;
    QByteArray m_pendingData;

    QString m_host;
    QString m_user;
    QString m_pass;
    QString m_currentRemotePath;
    QString m_pendingRemotePath;
    State m_state;
    qint64 m_uploadOffset;
    QFtpClientLogger* m_logger;
};

} // namespace QNetUtils
