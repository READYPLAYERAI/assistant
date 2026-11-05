#pragma once

#include <QJsonArray>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class OpenRouterClient : public QObject {
    Q_OBJECT
public:
    explicit OpenRouterClient(QObject *parent = nullptr);
    ~OpenRouterClient() override;

    bool isStreaming() const;
    void sendChat(const QString &apiKey, const QString &model, const QJsonArray &messages);
    void cancel();

signals:
    void streamStarted();
    void contentDelta(const QString &text);
    void streamFinished(const QString &fullText);
    void streamFailed(const QString &errorMessage);

private:
    void onReadyRead();
    void onFinished();
    void drainBuffer(bool flushTail);
    void handleLine(const QString &line);
    void fail(const QString &message);
    void cleanup();

    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    QString m_buffer;
    QString m_accumulated;
    QString m_networkError;
    bool m_done = false;
    bool m_cancelled = false;
};
