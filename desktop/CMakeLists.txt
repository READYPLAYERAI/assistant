cmake_minimum_required(VERSION 3.21)
project(llmchat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

qt_add_executable(llmchat
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/Settings.h
    src/Settings.cpp
    src/OpenRouterClient.h
    src/OpenRouterClient.cpp
)

target_link_libraries(llmchat PRIVATE Qt6::Widgets Qt6::Network)

install(TARGETS llmchat RUNTIME DESTINATION bin)
install(FILES resources/llmchat.desktop DESTINATION share/applications)
