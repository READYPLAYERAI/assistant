#include "MainWindow.h"

#include "OpenRouterClient.h"
#include "Settings.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStatusBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("LLM Chat"));
    resize(900, 700);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // Settings row
    auto *settingsLayout = new QHBoxLayout;
    m_keyEdit = new QLineEdit(central);
    m_keyEdit->setObjectName(QStringLiteral("keyEdit"));
    m_keyEdit->setEchoMode(QLineEdit::Password);
    m_keyEdit->setPlaceholderText(tr("OpenRouter API key"));
    m_keyEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_modelEdit = new QLineEdit(central);
    m_modelEdit->setObjectName(QStringLiteral("modelEdit"));
    m_modelEdit->setText("deepseek/deepseek-flash-latest");
    m_modelEdit->setPlaceholderText(tr("Model id"));
    m_modelEdit->setMinimumWidth(220);

    m_rememberCheck = new QCheckBox(tr("Remember key"), central);
    m_rememberCheck->setObjectName(QStringLiteral("rememberCheck"));
    m_rememberCheck->setChecked(false);

    settingsLayout->addWidget(m_keyEdit);
    settingsLayout->addWidget(m_modelEdit);
    settingsLayout->addWidget(m_rememberCheck);
    mainLayout->addLayout(settingsLayout);

    // Transcript
    m_transcript = new QPlainTextEdit(central);
    m_transcript->setObjectName(QStringLiteral("transcript"));
    m_transcript->setReadOnly(true);
    m_transcript->setUndoRedoEnabled(false);
    m_transcript->setPlaceholderText(tr("Conversation will appear here."));
    m_transcript->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_transcript->setCenterOnScroll(false);
    mainLayout->addWidget(m_transcript, 1);

    // Input row
    auto *inputLayout = new QHBoxLayout;
    m_input = new QPlainTextEdit(central);
    m_input->setObjectName(QStringLiteral("input"));
    m_input->setFixedHeight(72);
    m_input->setPlaceholderText(tr("Message… (Enter to send, Shift+Enter for newline)"));

    auto *buttonLayout = new QVBoxLayout;
    m_sendButton = new QPushButton(tr("Send"), central);
    m_sendButton->setObjectName(QStringLiteral("sendButton"));
    m_stopButton = new QPushButton(tr("Stop"), central);
    m_stopButton->setObjectName(QStringLiteral("stopButton"));
    m_stopButton->setEnabled(false);
    m_clearButton = new QPushButton(tr("Clear"), central);
    m_clearButton->setObjectName(QStringLiteral("clearButton"));

    buttonLayout->addWidget(m_sendButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_clearButton);

    inputLayout->addWidget(m_input, 1);
    inputLayout->addLayout(buttonLayout, 0);
    mainLayout->addLayout(inputLayout);

    setCentralWidget(central);

    statusBar()->showMessage(tr("Ready"));

    m_keyEdit->setText(Settings::instance().apiKey());
    m_modelEdit->setText(Settings::instance().model());
    m_rememberCheck->setChecked(Settings::instance().rememberKey());

    connect(m_modelEdit, &QLineEdit::editingFinished, this, &MainWindow::saveSettings);
    connect(m_rememberCheck, &QCheckBox::toggled, this, &MainWindow::saveSettings);

    m_client = new OpenRouterClient(this);
    connect(m_client, &OpenRouterClient::streamStarted, this, &MainWindow::onStreamStarted);
    connect(m_client, &OpenRouterClient::contentDelta, this, &MainWindow::onContentDelta);
    connect(m_client, &OpenRouterClient::streamFinished, this, &MainWindow::onStreamFinished);
    connect(m_client, &OpenRouterClient::streamFailed, this, &MainWindow::onStreamFailed);

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopStreaming);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::clearConversation);

    m_input->installEventFilter(this);
}

void MainWindow::saveSettings()
{
    Settings &s = Settings::instance();
    s.setModel(m_modelEdit->text().trimmed());
    s.setRememberKey(m_rememberCheck->isChecked());
    s.setApiKey(m_keyEdit->text().trimmed());
    s.save();
}

void MainWindow::sendMessage()
{
    if (m_assistantStreaming)
        return;

    const QString key = m_keyEdit->text().trimmed();
    const QString model = m_modelEdit->text().trimmed();
    const QString text = m_input->toPlainText().trimmed();
    if (key.isEmpty()) {
        statusBar()->showMessage(tr("Enter an OpenRouter API key"), 5000);
        m_keyEdit->setFocus();
        return;
    }
    if (model.isEmpty()) {
        statusBar()->showMessage(tr("Enter a model id"), 5000);
        m_modelEdit->setFocus();
        return;
    }
    if (text.isEmpty())
        return;

    saveSettings();

    QTextCursor cursor(m_transcript->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral("\nYou:\n") + text + QStringLiteral("\n\nAssistant:\n"));
    m_assistantStartPos = cursor.position();

    QJsonObject userMessage;
    userMessage.insert(QStringLiteral("role"), QStringLiteral("user"));
    userMessage.insert(QStringLiteral("content"), text);
    m_messages.append(userMessage);

    m_input->clear();
    m_stopRequested = false;
    setBusy(true);
    statusBar()->showMessage(tr("Contacting %1…").arg(model));
    m_client->sendChat(key, model, m_messages);
}

void MainWindow::stopStreaming()
{
    if (!m_client->isStreaming())
        return;

    m_stopRequested = true;
    m_stopButton->setEnabled(false);
    m_client->cancel();
}

void MainWindow::clearConversation()
{
    if (m_assistantStreaming)
        return;

    m_transcript->clear();
    m_messages = QJsonArray();
    m_assistantStartPos = 0;
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::onStreamStarted()
{
    statusBar()->showMessage(tr("Streaming…"));
}

void MainWindow::onContentDelta(const QString &text)
{
    appendAssistantDelta(text);
}

void MainWindow::onStreamFinished(const QString &fullText)
{
    const bool stopped = m_stopRequested;
    m_stopRequested = false;

    const int endBefore = m_transcript->document()->characterCount() - 1;

    QTextCursor cursor(m_transcript->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(stopped ? QStringLiteral("\n[stopped]\n") : QStringLiteral("\n"));

    if (stopped) {
        QTextCursor selection(m_transcript->document());
        selection.setPosition(m_assistantStartPos);
        selection.setPosition(endBefore, QTextCursor::KeepAnchor);
        const QString partial = selection.selectedText().trimmed();
        if (!partial.isEmpty()) {
            QJsonObject assistantMessage;
            assistantMessage.insert(QStringLiteral("role"), QStringLiteral("assistant"));
            assistantMessage.insert(QStringLiteral("content"), partial);
            m_messages.append(assistantMessage);
        }
    } else if (!fullText.trimmed().isEmpty()) {
        QJsonObject assistantMessage;
        assistantMessage.insert(QStringLiteral("role"), QStringLiteral("assistant"));
        assistantMessage.insert(QStringLiteral("content"), fullText);
        m_messages.append(assistantMessage);
    }

    setBusy(false);
    statusBar()->showMessage(stopped ? tr("Stopped") : tr("Ready"));
}

void MainWindow::onStreamFailed(const QString &message)
{
    m_stopRequested = false;

    if (message == QStringLiteral("Cancelled")) {
        const int endBefore = m_transcript->document()->characterCount() - 1;

        QTextCursor cursor(m_transcript->document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(QStringLiteral("\n[stopped]\n"));

        QTextCursor selection(m_transcript->document());
        selection.setPosition(m_assistantStartPos);
        selection.setPosition(endBefore, QTextCursor::KeepAnchor);
        const QString partial = selection.selectedText().trimmed();
        if (!partial.isEmpty()) {
            QJsonObject assistantMessage;
            assistantMessage.insert(QStringLiteral("role"), QStringLiteral("assistant"));
            assistantMessage.insert(QStringLiteral("content"), partial);
            m_messages.append(assistantMessage);
        }

        setBusy(false);
        statusBar()->showMessage(tr("Stopped"));
        return;
    }

    QTextCursor cursor(m_transcript->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral("\n[error] ") + message + QStringLiteral("\n"));

    setBusy(false);
    statusBar()->showMessage(tr("Request failed: %1").arg(message), 10000);
}

void MainWindow::setBusy(bool busy)
{
    m_assistantStreaming = busy;
    m_sendButton->setEnabled(!busy);
    m_stopButton->setEnabled(busy);
    m_input->setEnabled(!busy);
    m_clearButton->setEnabled(!busy);
    if (!busy)
        m_input->setFocus();
}

void MainWindow::appendAssistantDelta(const QString &text)
{
    QScrollBar *sb = m_transcript->verticalScrollBar();
    const bool atBottom = sb->value() >= sb->maximum() - 4;

    QTextCursor cursor(m_transcript->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);

    if (atBottom)
        sb->setValue(sb->maximum());
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        const bool isEnter = keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter;
        if (isEnter && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            sendMessage();
            return true; // consume: no newline inserted
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (m_client && m_client->isStreaming())
        m_client->cancel();
    QMainWindow::closeEvent(event);
}
