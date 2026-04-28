cmake_minimum_required(VERSION 3.16)

project(Schedule_Search LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Schedule_Search main.cpp
    Step1.cpp
    Step2.cpp
    Step3.cpp
)

include(GNUInstallDirs)
install(TARGETS Schedule_Search
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
