# ============================================
# Communication B (αB)
# ============================================
# L#
#   - ΅ CAN / Serial / TCP Iα¥γ
#   - Ρ¥6Λpn'
#   - Η signal ¥60„pnΡΩ Controller
#
# VΔ
#   - κύV Qt::Core, Qt::Network, Qt::SerialPort
#   - ύV Controller / UI / Parser
#   - Η signal/slot  Controller α
# ============================================

set(COMMUNICATION_SOURCES
    canmanager.cpp
    canmanager.h
)

# ϊY“
add_library(communication_layer STATIC ${COMMUNICATION_SOURCES})

# ώ¥ Qt Qά2γ!W
target_link_libraries(communication_layer PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::SerialPort
)

# Ύn4‡φ+ο„
target_include_directories(communication_layer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
