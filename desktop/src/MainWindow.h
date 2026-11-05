#pragma once

#include <QMainWindow>

class QCheckBox;
class QCloseEvent;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void saveSettings();

private:
    QLineEdit *m_keyEdit = nullptr;
    QLineEdit *m_modelEdit = nullptr;
    QCheckBox *m_rememberCheck = nullptr;
    QPlainTextEdit *m_transcript = nullptr;
    QPlainTextEdit *m_input = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_clearButton = nullptr;
};
