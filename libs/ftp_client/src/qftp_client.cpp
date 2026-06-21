#include "qftp_client.h"
#include <QHostAddress>
#include <QRegularExpression>
#include <QDebug>

namespace QNetUtils {

QFtpClient::QFtpClient(QObject *parent)
    : QObject(parent),
      m_controlSocket(new QTcpSocket(this)),
      m_dataSocket(nullptr),
      m_timeout(new QTimer(this)),
      m_file(nullptr),
      m_state(State::Idle),
      m_uploadOffset(0),
      m_logger(nullptr)
{
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(30000);

    connect(m_controlSocket, &QTcpSocket::readyRead, 
            this, &QFtpClient::onControlReadyRead);
    connect(m_controlSocket, &QTcpSocket::connected, 
            this, &QFtpClient::onControlConnected);
    connect(m_controlSocket, 
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &QFtpClient::onControlError);
    connect(m_controlSocket, &QTcpSocket::disconnected, 
            this, &QFtpClient::onSocketDisconnected);
    connect(m_timeout, &QTimer::timeout, 
            this, &QFtpClient::onTimeout);
}

QFtpClient::~QFtpClient()
{
    disconnectFromServer();
}

void QFtpClient::log(const QString& msg)
{
    qDebug() << "[FTP]" << msg;
    if (m_logger) {
        m_logger->log("[FTP] " + msg);
    }
}

void QFtpClient::logError(const QString& msg)
{
    qWarning() << "[FTP ERROR]" << msg;
    if (m_logger) {
        m_logger->logError("[FTP ERROR] " + msg);
    }
}

void QFtpClient::connectToServer(const QString &host, quint16 port,
                                  const QString &user, const QString &pass)
{
    if (m_state != State::Idle && m_state != State::Disconnected) {
        disconnectFromServer();
    }
    
    m_host = host;
    m_user = user;
    m_pass = pass;
    m_state = State::Connecting;
    m_currentRemotePath = "/";

    log(QString("Connecting to %1:%2").arg(host).arg(port));
    m_controlSocket->connectToHost(host, port);
    m_timeout->start();
}

void QFtpClient::disconnectFromServer()
{
    if (m_state == State::Idle) return;

    if (m_state == State::Transferring) {
        abortCurrentOperation();
    }

    if (m_controlSocket->state() == QAbstractSocket::ConnectedState) {
        sendCommand("QUIT");
    }

    m_controlSocket->disconnectFromHost();
    resetState();
    emit disconnected();
}

void QFtpClient::uploadFile(const QString &localPath, const QString &remotePath)
{
    if (!isConnected()) {
        QString err("Not connected to FTP server");
        logError(err);
        emit errorOccurred(err);
        return;
    }

    m_file = new QFile(localPath, this);
    if (!m_file->open(QIODevice::ReadOnly)) {
        QString err("Failed to open local file: " + localPath);
        logError(err);
        emit errorOccurred(err);
        delete m_file;
        m_file = nullptr;
        return;
    }

    m_pendingRemotePath = remotePath;
    log(QString("Preparing upload: %1 -> %2").arg(localPath, remotePath));
    sendCommand("TYPE I");
}

void QFtpClient::changeDirectory(const QString &path)
{
    if (!isConnected()) return;
    sendCommand("CWD " + path);
}

void QFtpClient::sendCommand(const QString &cmd)
{
    if (m_controlSocket->state() != QAbstractSocket::ConnectedState) {
        logError("Control socket not connected");
        return;
    }
    QByteArray data = cmd.toUtf8() + "\r\n";
    log(QString("CMD: %1").arg(cmd.trimmed()));
    m_controlSocket->write(data);
    m_timeout->start();
}

void QFtpClient::onControlConnected()
{
    log("TCP Connected");
}

void QFtpClient::onControlReadyRead()
{
    m_timeout->stop();
    QByteArray data = m_controlSocket->readAll();
    m_pendingData.append(data);

    while (true) {
        int idx = m_pendingData.indexOf("\r\n");
        if (idx == -1) break;
        QByteArray line = m_pendingData.left(idx);
        m_pendingData.remove(0, idx + 2);
        if (line.isEmpty()) continue;

        processReply(QString::fromUtf8(line));

        if (line.length() >= 4 && line[3] == ' ') {
            break;
        }
    }

    if (!m_pendingData.isEmpty()) {
        m_timeout->start();
    }
}

void QFtpClient::processReply(const QString &reply)
{
    log(QString("Reply: %1 | State: %2").arg(reply.trimmed()).arg(static_cast<int>(m_state)));

    if (reply.length() < 3) return;
    int code = reply.left(3).toInt();
    QString text = reply.mid(4).trimmed();

    switch (m_state) {
    case State::Connecting:
        if (code == 220) {
            sendCommand("USER " + m_user);
            m_state = State::WaitingUser;
        } else {
            emit errorOccurred("Connection error: " + text);
            disconnectFromServer();
        }
        break;

    case State::WaitingUser:
        if (code == 331) {
            sendCommand("PASS " + m_pass);
            m_state = State::WaitingPass;
        } else if (code == 230) {
            m_state = State::LoggedIn;
            emit connected();
        } else {
            emit errorOccurred("User error: " + text);
            disconnectFromServer();
        }
        break;

    case State::WaitingPass:
        if (code == 230) {
            m_state = State::LoggedIn;
            emit connected();
        } else {
            emit errorOccurred("Password error: " + text);
            disconnectFromServer();
        }
        break;

    case State::LoggedIn:
        if (code == 200) {
            if (!m_pendingRemotePath.isEmpty() && m_file) {
                sendCommand("PASV");
                m_state = State::WaitingPasv;
            }
        } else if (code == 227) {
            QRegularExpression re("\\(([^)]+)\\)");
            auto match = re.match(reply);
            if (!match.hasMatch()) {
                emit errorOccurred("Failed to parse PASV response");
                abortCurrentOperation();
                break;
            }
            QStringList parts = match.captured(1).split(",");
            if (parts.size() < 6) {
                emit errorOccurred("Invalid PASV response format");
                abortCurrentOperation();
                break;
            }
            QString host = QString("%1.%2.%3.%4")
                               .arg(parts[0], parts[1], parts[2], parts[3]);
            quint16 port = parts[4].toInt() * 256 + parts[5].toInt();
            openDataConnection(host, port);
        } else if (code == 150 || code == 125) {
            log(QString("Transfer starting (code %1)").arg(code));
            m_state = State::Transferring;
            if (m_dataSocket &&
                m_dataSocket->state() == QAbstractSocket::ConnectedState) {
                startUpload();
            }
        } else if (code == 250) {
            m_currentRemotePath = m_pendingRemotePath;
        } else if (code >= 400) {
            emit errorOccurred("Command error: " + text);
            abortCurrentOperation();
        }
        break;

    case State::WaitingPasv:
        if (code == 227) {
            QRegularExpression re("\\(([^)]+)\\)");
            auto match = re.match(reply);
            if (match.hasMatch()) {
                QStringList parts = match.captured(1).split(",");
                if (parts.size() >= 6) {
                    QString host = QString("%1.%2.%3.%4")
                                       .arg(parts[0], parts[1], parts[2], parts[3]);
                    quint16 port = parts[4].toInt() * 256 + parts[5].toInt();
                    openDataConnection(host, port);
                }
            }
        } else if (code >= 400) {
            emit errorOccurred("PASV error: " + text);
            abortCurrentOperation();
        }
        break;

    case State::WaitingTransfer:
        // 150 = File status okay; about to open data connection
        // 125 = Data connection already open; transfer starting
        if (code == 150 || code == 125) {
            log(QString("Transfer starting (code %1)").arg(code));
            m_state = State::Transferring;
            if (m_dataSocket &&
                m_dataSocket->state() == QAbstractSocket::ConnectedState) {
                startUpload();
            }
        } else if (code >= 400) {
            emit errorOccurred("Transfer error: " + text);
            abortCurrentOperation();
        }
        break;

    case State::Transferring:
        if (code == 226) {
            finalizeTransfer();
        } else if (code >= 400) {
            emit errorOccurred("Completion error: " + text);
            abortCurrentOperation();
        }
        break;

    default:
        break;
    }
}

void QFtpClient::openDataConnection(const QString &host, quint16 port)
{
    if (m_dataSocket) {
        m_dataSocket->deleteLater();
        m_dataSocket = nullptr;
    }
    m_dataSocket = new QTcpSocket(this);

    connect(m_dataSocket, &QTcpSocket::readyRead, 
            this, &QFtpClient::onDataReadyRead);
    connect(m_dataSocket, &QTcpSocket::connected, this, [this]() {
        log("Data socket connected");
        sendCommand("STOR " + m_pendingRemotePath);
        m_state = State::WaitingTransfer;
    });
    connect(m_dataSocket, &QTcpSocket::disconnected, 
            this, &QFtpClient::onDataSocketDisconnected);
    connect(m_dataSocket, 
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            [this](QAbstractSocket::SocketError) {
                emit errorOccurred("Data socket error: " + m_dataSocket->errorString());
                abortCurrentOperation();
            });
    connect(m_dataSocket, &QTcpSocket::bytesWritten, 
            this, &QFtpClient::onDataBytesWritten);

    log(QString("Connecting data to %1:%2").arg(host).arg(port));
    m_dataSocket->connectToHost(host, port);
}

void QFtpClient::startUpload()
{
    if (!m_file || !m_dataSocket) return;
    log("Starting upload stream");
    m_uploadOffset = 0;
    sendDataChunk();
}

void QFtpClient::sendDataChunk()
{
    if (!m_file || !m_dataSocket) return;

    if (m_file->atEnd()) {
        log("File fully sent, closing data socket");
        m_dataSocket->disconnectFromHost();
        return;
    }

    QByteArray chunk = m_file->read(4096);
    if (chunk.isEmpty()) return;

    qint64 written = m_dataSocket->write(chunk);
    if (written > 0) {
        m_uploadOffset += written;
        emit uploadProgress(m_uploadOffset, m_file->size());
    }
}

void QFtpClient::onDataBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
    if (m_state == State::Transferring) {
        sendDataChunk();
    }
}

void QFtpClient::onDataReadyRead()
{
    if (!m_dataSocket || !m_file) return;
    m_dataSocket->readAll();
}

void QFtpClient::onDataSocketDisconnected()
{
    if (m_state == State::Transferring) {
        if (m_file) {
            finalizeTransfer();
        }
    }
    if (m_dataSocket) {
        m_dataSocket->deleteLater();
        m_dataSocket = nullptr;
    }
}

void QFtpClient::finalizeTransfer()
{
    log("Finalizing transfer");
    if (m_file) {
        m_file->close();
        QString path = m_pendingRemotePath;
        delete m_file;
        m_file = nullptr;
        m_pendingRemotePath.clear();
        emit uploadFinished(path);
    }
    m_state = State::LoggedIn;
}

void QFtpClient::abortCurrentOperation()
{
    if (m_dataSocket || m_file) {
        log("Aborting current operation");
        if (m_controlSocket->state() == QAbstractSocket::ConnectedState) {
            sendCommand("ABOR");
        }
    }

    if (m_dataSocket) {
        m_dataSocket->disconnectFromHost();
        m_dataSocket->deleteLater();
        m_dataSocket = nullptr;
    }
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    m_pendingRemotePath.clear();
    m_state = State::LoggedIn;
}

void QFtpClient::resetState()
{
    m_state = State::Idle;
    m_pendingData.clear();

    if (m_dataSocket) {
        m_dataSocket->disconnectFromHost();
        m_dataSocket->deleteLater();
        m_dataSocket = nullptr;
    }
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    m_pendingRemotePath.clear();
}

void QFtpClient::onControlError(QAbstractSocket::SocketError)
{
    emit errorOccurred("Control error: " + m_controlSocket->errorString());
    disconnectFromServer();
}

void QFtpClient::onSocketDisconnected()
{
    resetState();
    emit disconnected();
}

void QFtpClient::onTimeout()
{
    emit errorOccurred("FTP operation timeout");
    disconnectFromServer();
}

}
