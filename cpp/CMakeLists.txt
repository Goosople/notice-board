cmake_minimum_required(VERSION 3.16)
project(NoticeBoard VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(notice_board
    src/main.cpp
    src/ntfylistener.cpp
    src/noticeboard.cpp
    src/admindialog.cpp
)

target_link_libraries(notice_board PRIVATE Qt6::Widgets Qt6::Network)

install(TARGETS notice_board RUNTIME DESTINATION bin)
