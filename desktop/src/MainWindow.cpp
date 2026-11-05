#include "MainWindow.h"

#include "Settings.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QStatusBar>
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
    m_keyEdit->setEchoMode(QLineEdit::Password);
    m_keyEdit->setPlaceholderText(tr("OpenRouter API key"));
    m_keyEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_modelEdit = new QLineEdit(central);
    m_modelEdit->setText("deepseek/deepseek-flash-latest");
    m_modelEdit->setPlaceholderText(tr("Model id"));
    m_modelEdit->setMinimumWidth(220);

    m_rememberCheck = new QCheckBox(tr("Remember key"), central);
    m_rememberCheck->setChecked(false);

    settingsLayout->addWidget(m_keyEdit);
    settingsLayout->addWidget(m_modelEdit);
    settingsLayout->addWidget(m_rememberCheck);
    mainLayout->addLayout(settingsLayout);

    // Transcript
    m_transcript = new QPlainTextEdit(central);
    m_transcript->setReadOnly(true);
    m_transcript->setUndoRedoEnabled(false);
    m_transcript->setPlaceholderText(tr("Conversation will appear here."));
    m_transcript->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    mainLayout->addWidget(m_transcript, 1);

    // Input row
    auto *inputLayout = new QHBoxLayout;
    m_input = new QPlainTextEdit(central);
    m_input->setFixedHeight(72);
    m_input->setPlaceholderText(tr("Message… (Enter to send, Shift+Enter for newline)"));

    auto *buttonLayout = new QVBoxLayout;
    m_sendButton = new QPushButton(tr("Send"), central);
    m_stopButton = new QPushButton(tr("Stop"), central);
    m_stopButton->setEnabled(false);
    m_clearButton = new QPushButton(tr("Clear"), central);

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
}

void MainWindow::saveSettings()
{
    Settings &s = Settings::instance();
    s.setModel(m_modelEdit->text().trimmed());
    s.setRememberKey(m_rememberCheck->isChecked());
    s.setApiKey(m_keyEdit->text().trimmed());
    s.save();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}
