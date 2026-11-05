#pragma once

#include <QJsonArray>
#include <QMainWindow>

class QCheckBox;
class QCloseEvent;
class QEvent;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class OpenRouterClient;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void saveSettings();
    void sendMessage();
    void stopStreaming();
    void clearConversation();
    void onStreamStarted();
    void onContentDelta(const QString &text);
    void onStreamFinished(const QString &fullText);
    void onStreamFailed(const QString &errorMessage);

private:
    void setBusy(bool busy);
    void appendAssistantDelta(const QString &text);

    QLineEdit *m_keyEdit = nullptr;
    QLineEdit *m_modelEdit = nullptr;
    QCheckBox *m_rememberCheck = nullptr;
    QPlainTextEdit *m_transcript = nullptr;
    QPlainTextEdit *m_input = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_clearButton = nullptr;

    OpenRouterClient *m_client = nullptr;
    QJsonArray m_messages;
    bool m_assistantStreaming = false;
    int m_assistantStartPos = 0;
    bool m_stopRequested = false;
};
