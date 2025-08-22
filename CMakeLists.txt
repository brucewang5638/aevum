cmake_minimum_required(VERSION 3.16)

project(WorkstationWellnessElf VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/core/ActivityMonitor.cpp
    src/core/HealthEngine.cpp
    src/core/ConfigManager.cpp
    src/core/DataAnalyzer.cpp
    src/ui/SystemTrayIcon.cpp
    src/ui/SettingsDialog.cpp
    src/ui/NotificationWidget.cpp
    src/ui/StatisticsPanel.cpp
    src/utils/Logger.cpp
    src/utils/SystemUtils.cpp
)

set(HEADERS
    include/core/ActivityMonitor.h
    include/core/HealthEngine.h
    include/core/ConfigManager.h
    include/core/DataAnalyzer.h
    include/ui/SystemTrayIcon.h
    include/ui/SettingsDialog.h
    include/ui/NotificationWidget.h
    include/ui/StatisticsPanel.h
    include/utils/Logger.h
    include/utils/SystemUtils.h
)

include_directories(include)

add_executable(WorkstationWellnessElf ${SOURCES} ${HEADERS})

qt_add_resources(WorkstationWellnessElf "resources"
    PREFIX "/"
    FILES
        resources/icons/app.png
        resources/icons/tray.png
        resources/icons/notification.png
)

target_link_libraries(WorkstationWellnessElf PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)

if(WIN32)
    target_link_libraries(WorkstationWellnessElf PRIVATE user32 Winmm Pdh)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(WorkstationWellnessElf PRIVATE X11 Xss)
endif()