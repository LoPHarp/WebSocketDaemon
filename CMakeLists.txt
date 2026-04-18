cmake_minimum_required(VERSION 3.16)

project(WebSocketDaemon LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets)

add_executable(WebSocketDaemon
  main.cpp
  GlobalSettings.h
  pushserver.h pushserver.cpp
  logger.h
  logger.cpp
  configmanager.h
  configmanager.cpp
)

target_link_libraries(WebSocketDaemon PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::WebSockets
)

include(GNUInstallDirs)
install(TARGETS WebSocketDaemon
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
