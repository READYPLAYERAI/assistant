#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("llmchat");
    QApplication::setOrganizationName("llmchat");
    QApplication::setApplicationDisplayName("LLM Chat");
    QApplication::setDesktopFileName("llmchat");

    MainWindow window;
    window.show();

    return app.exec();
}
