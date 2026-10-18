#ifndef MOTION_SERVER_H
#define MOTION_SERVER_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

class QTcpServer;
class QTcpSocket;

namespace pose {

class MotionEngine;

class MotionServer : public QObject {
    Q_OBJECT

public:
    explicit MotionServer(MotionEngine* engine, QObject* parent = nullptr);
    ~MotionServer() override;

    /// Starts listening for JSON commands on 127.0.0.1:port (default 9876).
    bool start(quint16 port = 9876);
    void stop();

    bool isListening() const;
    quint16 serverPort() const;

signals:
    void commandExecuted(const QString& command);
    void playRequested();
    void pauseRequested();
    void stopRequested();
    void seekRequested(float seconds);
    void cameraFollowRequested(bool enabled);
    void loopRequested(bool enabled);
    void loadScriptRequested(const QString& jsonContent);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    QByteArray handleCommand(const QByteArray& requestLine);

    MotionEngine*      m_engine = nullptr;
    QTcpServer*        m_server = nullptr;
    QList<QTcpSocket*> m_clients;
};

} // namespace pose

#endif // MOTION_SERVER_H
