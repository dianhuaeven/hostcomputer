# ============================================
# Model B (pn!‹B)
# ============================================
# L#
#   - : UI Ğ›ïÑš„pn!‹ (QObject + Q_PROPERTY)
#   - Ç signal/slot ô° UI
#
# VÄ
#   - êıV Qt::Core
#   - ıV Parser / Communication / Controller
#   - 1 Controller #ô° Model pn
# ============================================

set(MODEL_SOURCES
    motormodel.cpp
    motormodel.h
)

# úY“
add_library(model_layer STATIC ${MODEL_SOURCES})

# ş¥ Qt::Core
target_link_libraries(model_layer PUBLIC
    Qt6::Core
)

# ¾n4‡ö+ï„
target_include_directories(model_layer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
