#include "OpenRouterClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QtGlobal>

OpenRouterClient::OpenRouterClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

OpenRouterClient::~OpenRouterClient()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

bool OpenRouterClient::isStreaming() const
{
    return m_reply != nullptr;
}

void OpenRouterClient::sendChat(const QString &apiKey, const QString &model, const QJsonArray &messages)
{
    if (isStreaming())
        return;

    m_buffer.clear();
    m_accumulated.clear();
    m_networkError.clear();
    m_done = false;
    m_cancelled = false;

    const QString endpoint = qEnvironmentVariable(
        "LLMCHAT_OPENROUTER_URL",
        QStringLiteral("https://openrouter.ai/api/v1/chat/completions"));

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), model);
    payload.insert(QStringLiteral("messages"), messages);
    payload.insert(QStringLiteral("stream"), true);
    const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QNetworkRequest request{QUrl(endpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
    request.setRawHeader("X-Title", "LLM Chat");
    request.setRawHeader("HTTP-Referer", "https://localhost");
    request.setTransferTimeout(300000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_nam->post(request, body);
    connect(m_reply, &QNetworkReply::readyRead, this, &OpenRouterClient::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &OpenRouterClient::onFinished);
    connect(m_reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError) {
        if (m_reply)
            m_networkError = m_reply->errorString();
    });

    emit streamStarted();
}

void OpenRouterClient::cancel()
{
    if (!m_reply || m_done)
        return;

    m_cancelled = true;
    m_done = true;
    const QString text = m_accumulated;

    m_reply->abort();
    cleanup();

    if (!text.isEmpty())
        emit streamFinished(text);
    else
        emit streamFailed(QStringLiteral("Cancelled"));
}

void OpenRouterClient::onReadyRead()
{
    if (!m_reply)
        return;

    m_buffer += QString::fromUtf8(m_reply->readAll());
    drainBuffer(false);
}

void OpenRouterClient::onFinished()
{
    if (!m_reply)
        return;

    if (m_cancelled) {
        cleanup();
        return;
    }

    const QByteArray remainder = m_reply->readAll();
    if (!remainder.isEmpty())
        m_buffer += QString::fromUtf8(remainder);

    const QByteArray rawBody = m_buffer.toUtf8();

    drainBuffer(true);
    if (!m_reply)
        return;

    const QNetworkReply::NetworkError netError = m_reply->error();
    const int httpStatus = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_done) {
        cleanup();
        return;
    }

    if (netError != QNetworkReply::NoError || (httpStatus != 0 && httpStatus != 200)) {
        QString message = m_networkError;
        const QJsonObject obj = QJsonDocument::fromJson(rawBody).object();
        if (obj.contains(QStringLiteral("error"))) {
            const QJsonObject error = obj.value(QStringLiteral("error")).toObject();
            const QString errMessage = error.value(QStringLiteral("message")).toString();
            if (!errMessage.isEmpty())
                message = errMessage;
        }
        if (message.isEmpty())
            message = m_reply->errorString();
        if (httpStatus > 0)
            message = QStringLiteral("HTTP %1: %2").arg(httpStatus).arg(message);
        fail(message);
        return;
    }

    m_done = true;
    emit streamFinished(m_accumulated);
    cleanup();
}

void OpenRouterClient::drainBuffer(bool flushTail)
{
    int nl = m_buffer.indexOf(QLatin1Char('\n'));
    while (nl >= 0) {
        QString line = m_buffer.left(nl);
        m_buffer.remove(0, nl + 1);
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (!line.trimmed().isEmpty())
            handleLine(line.trimmed());
        if (!m_reply)
            return;
        nl = m_buffer.indexOf(QLatin1Char('\n'));
    }

    if (flushTail) {
        const QString tail = m_buffer.trimmed();
        m_buffer.clear();
        if (!tail.isEmpty())
            handleLine(tail);
    }
}

void OpenRouterClient::handleLine(const QString &line)
{
    if (line.startsWith(QLatin1Char(':'))
        || line.startsWith(QStringLiteral("event:"))
        || line.startsWith(QStringLiteral("id:"))
        || line.startsWith(QStringLiteral("retry:"))) {
        return;
    }

    if (!line.startsWith(QStringLiteral("data:")))
        return;

    const QString payload = line.mid(5).trimmed();
    if (payload == QStringLiteral("[DONE]")) {
        if (m_done)
            return;
        m_done = true;
        emit streamFinished(m_accumulated);
        cleanup();
        return;
    }

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    const QJsonObject obj = doc.object();

    if (obj.contains(QStringLiteral("error"))) {
        const QJsonObject error = obj.value(QStringLiteral("error")).toObject();
        QString message = error.value(QStringLiteral("message")).toString();
        const QJsonValue code = error.value(QStringLiteral("code"));
        QString codeText;
        if (code.isDouble())
            codeText = QString::number(code.toInt());
        else if (code.isString())
            codeText = code.toString();
        if (!codeText.isEmpty())
            message += QStringLiteral(" (") + codeText + QStringLiteral(")");
        if (message.isEmpty())
            message = QStringLiteral("OpenRouter error");
        fail(message);
        return;
    }

    const QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
    if (!choices.isEmpty()) {
        const QJsonObject delta = choices.at(0).toObject().value(QStringLiteral("delta")).toObject();
        const QJsonValue content = delta.value(QStringLiteral("content"));
        if (content.isString()) {
            const QString text = content.toString();
            if (!text.isEmpty()) {
                m_accumulated += text;
                emit contentDelta(text);
            }
        }
    }
}

void OpenRouterClient::fail(const QString &message)
{
    if (m_done)
        return;
    m_done = true;
    emit streamFailed(message);
    cleanup();
}

void OpenRouterClient::cleanup()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_buffer.clear();
}
