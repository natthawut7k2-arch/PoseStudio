#include "motionserver.h"
#include "motionengine.h"

#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTcpServer>
#include <QTcpSocket>

namespace pose {

MotionServer::MotionServer(MotionEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine) {}

MotionServer::~MotionServer() {
    stop();
}

bool MotionServer::start(quint16 port) {
    if (m_server && m_server->isListening()) {
        return true;
    }

    if (!m_server) {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &MotionServer::onNewConnection);
    }

    bool ok = m_server->listen(QHostAddress::LocalHost, port);
    if (ok) {
        qInfo() << "[MotionServer] Listening on 127.0.0.1:" << port;
    } else {
        qWarning() << "[MotionServer] Failed to listen on port" << port << ":" << m_server->errorString();
    }
    return ok;
}

void MotionServer::stop() {
    if (m_server) {
        m_server->close();
    }
    for (QTcpSocket* client : m_clients) {
        client->disconnect(this);
        client->close();
        client->deleteLater();
    }
    m_clients.clear();
}

bool MotionServer::isListening() const {
    return m_server && m_server->isListening();
}

quint16 MotionServer::serverPort() const {
    return m_server ? m_server->serverPort() : 0;
}

void MotionServer::onNewConnection() {
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket* client = m_server->nextPendingConnection();
        m_clients.append(client);

        connect(client, &QTcpSocket::readyRead, this, &MotionServer::onClientReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &MotionServer::onClientDisconnected);
    }
}

void MotionServer::onClientDisconnected() {
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        m_clients.removeOne(client);
        client->deleteLater();
    }
}

void MotionServer::onClientReadyRead() {
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    while (client->canReadLine()) {
        QByteArray line = client->readLine().trimmed();
        if (line.isEmpty()) continue;

        QByteArray response = handleCommand(line);
        client->write(response + '\n');
        client->flush();
    }
}

QByteArray MotionServer::handleCommand(const QByteArray& requestLine) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(requestLine, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QJsonObject err;
        err[QStringLiteral("ok")] = false;
        err[QStringLiteral("error")] = QStringLiteral("Malformed JSON command: %1").arg(parseError.errorString());
        return QJsonDocument(err).toJson(QJsonDocument::Compact);
    }

    QJsonObject req = doc.object();
    QString cmd = req.value(QStringLiteral("command")).toString().toLower();
    QJsonObject resp;

    if (!m_engine) {
        resp[QStringLiteral("ok")] = false;
        resp[QStringLiteral("error")] = QStringLiteral("MotionEngine not attached");
        return QJsonDocument(resp).toJson(QJsonDocument::Compact);
    }

    if (cmd == QLatin1String("get_status")) {
        resp[QStringLiteral("ok")] = true;
        resp[QStringLiteral("status")] = m_engine->isPlaying() ? QStringLiteral("playing") : QStringLiteral("paused");
        resp[QStringLiteral("time")] = m_engine->currentTime();
        resp[QStringLiteral("duration")] = m_engine->totalDuration();
        resp[QStringLiteral("camera_follow")] = m_engine->isCameraFollow();
        resp[QStringLiteral("loop")] = m_engine->isLoop();
        resp[QStringLiteral("current_motion")] = QString::fromStdString(m_engine->currentMotionName());
    } else if (cmd == QLatin1String("play")) {
        m_engine->play();
        resp[QStringLiteral("ok")] = true;
        resp[QStringLiteral("status")] = QStringLiteral("playing");
    } else if (cmd == QLatin1String("pause")) {
        m_engine->pause();
        resp[QStringLiteral("ok")] = true;
        resp[QStringLiteral("status")] = QStringLiteral("paused");
    } else if (cmd == QLatin1String("stop")) {
        m_engine->stop();
        resp[QStringLiteral("ok")] = true;
        resp[QStringLiteral("status")] = QStringLiteral("stopped");
        resp[QStringLiteral("time")] = 0.0;
    } else if (cmd == QLatin1String("seek")) {
        float time = static_cast<float>(req.value(QStringLiteral("time")).toDouble(0.0));
        m_engine->seek(time);
        resp[QStringLiteral("ok")] = true;
        resp[QStringLiteral("time")] = m_engine->currentTime();
    } else if (cmd == QLatin1String("set_camera_follow")) {
        bool follow = req.value(QStringLiteral("enabled")).toBool(true);
        m_engine->setCameraFollow(follow);
        resp[QStringLiteral("ok")] = true;
        resp[QStringLiteral("camera_follow")] = follow;
    } else if (cmd == QLatin1String("set_loop")) {
        bool loop = req.value(QStringLiteral("enabled")).toBool(true);
        m_engine->setLoop(loop);
        resp[QStringLiteral("ok")] = true;
        resp[QStringLiteral("loop")] = loop;
    } else if (cmd == QLatin1String("load_script")) {
        QJsonObject scriptObj = req.value(QStringLiteral("script")).toObject();
        QByteArray scriptBytes = QJsonDocument(scriptObj).toJson();
        bool loaded = m_engine->loadScriptFromJson(scriptBytes.toStdString());
        resp[QStringLiteral("ok")] = loaded;
        resp[QStringLiteral("duration")] = m_engine->totalDuration();
        if (!loaded) {
            resp[QStringLiteral("error")] = QStringLiteral("Failed to parse script sequence");
        }
    } else {
        resp[QStringLiteral("ok")] = false;
        resp[QStringLiteral("error")] = QStringLiteral("Unknown command '%1'").arg(cmd);
    }

    emit commandExecuted(cmd);
    return QJsonDocument(resp).toJson(QJsonDocument::Compact);
}

} // namespace pose
