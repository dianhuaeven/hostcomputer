cmake_minimum_required(VERSION 3.16)
project(hostcomputer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# Qt6 Modules
# ============================================
# 如果 UI 层用到了其他 Qt 模块（比如 QML、Charts）
# 你就只需要在这里添加 Qt6::<模块>
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Quick
    QuickWidgets
    Network
    SerialPort
    OpenGL
    OpenGLWidgets
    Charts
)

# ============================================
# 子目录（分层）
# 每一层一个文件夹，也对应一个 CMakeLists.txt
# ============================================
add_subdirectory(src/controller)     # 业务逻辑层
add_subdirectory(src/communication)  # 通信层（CAN/TCP/Serial）
add_subdirectory(src/parser)         # 协议解析层
add_subdirectory(src/model)          # 数据模型层
add_subdirectory(resources)          # 资源(qrc)

# ============================================
# 主程序 UI 层
# ============================================
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

qt_add_executable(hostcomputer
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# ============================================
# 链接各层模块
# ============================================
# 如果某一层需要依赖第三方库，写在对应层的 CMakeLists.txt 里
# 主工程只负责把层与层串起来
target_link_libraries(hostcomputer PRIVATE
    controller_layer
    communication_layer
    parser_layer
    model_layer
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::SerialPort
    Qt6::Network
    Qt6::Charts
)

qt_finalize_executable(hostcomputer)
