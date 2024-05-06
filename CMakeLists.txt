CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(project-721)

SET(CMAKE_CXX_FLAGS "-Wall -O3 -no-pie")
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC ON)

FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)

LINK_LIBRARIES(Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
LINK_LIBRARIES(-static-libgcc -static-libstdc++)

ADD_EXECUTABLE(${PROJECT_NAME}
        sleepsettingswindow.ui
        mainwindow.ui
        networksettingswindow.ui
        scannerwindow.ui
        basicsettingswindow.ui
        devicesettingswindow.ui
        addnewdevicedialog.ui
        addnewgroupdialog.ui
        addnewdevicedialog.h
        addnewgroupdialog.h
        asicdevice.h
        asictablewidget.h
        basicsettingswindow.h
        configurationholder.h
        devicesettingswindow.h
		main.hpp
        mainwindow.h
        networksettingswindow.h
        scannerwindow.h
        sleepsettingswindow.h
		logger.hpp
        main.cpp
        addnewdevicedialog.cpp
        addnewgroupdialog.cpp
        asicdevice.cpp
        asictablewidget.cpp
        basicsettingswindow.cpp
        configurationholder.cpp
        devicesettingswindow.cpp
        mainwindow.cpp
        networksettingswindow.cpp
        scannerwindow.cpp
        sleepsettingswindow.cpp
        logger.cpp
        ${PROJECT_NAME}.qrc
)
