#include "DisplayLayoutManager.h"
#include <QDebug>

DisplayLayoutManager::DisplayLayoutManager(int rows, int cols, QWidget *parent)
    : QWidget(parent)
    , m_rows(rows)
    , m_cols(cols)
    , m_gridLayout(new QGridLayout(this))
    , m_widgets(rows * cols, nullptr)
{
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(3);

    // 让每行每列均匀拉伸
    for (int r = 0; r < m_rows; ++r) {
        m_gridLayout->setRowStretch(r, 1);
    }
    for (int c = 0; c < m_cols; ++c) {
        m_gridLayout->setColumnStretch(c, 1);
    }
}

DisplayLayoutManager::~DisplayLayoutManager()
{
}

void DisplayLayoutManager::setWidget(int index, QWidget *widget)
{
    if (!isValidIndex(index) || !widget) {
        qWarning() << "DisplayLayoutManager::setWidget: invalid index" << index;
        return;
    }

    // 先移除旧的
    if (m_widgets[index]) {
        m_gridLayout->removeWidget(m_widgets[index]);
        m_widgets[index]->setParent(nullptr);
    }

    m_widgets[index] = widget;
    widget->setParent(this);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_gridLayout->addWidget(widget, toRow(index), toCol(index));
}

QWidget *DisplayLayoutManager::removeWidget(int index)
{
    if (!isValidIndex(index) || !m_widgets[index]) {
        return nullptr;
    }

    QWidget *w = m_widgets[index];
    m_gridLayout->removeWidget(w);
    w->setParent(nullptr);
    m_widgets[index] = nullptr;
    return w;
}

QWidget *DisplayLayoutManager::widget(int index) const
{
    if (!isValidIndex(index)) {
        return nullptr;
    }
    return m_widgets[index];
}

void DisplayLayoutManager::clearAll()
{
    for (int i = 0; i < count(); ++i) {
        if (m_widgets[i]) {
            m_gridLayout->removeWidget(m_widgets[i]);
            m_widgets[i]->setParent(nullptr);
            m_widgets[i] = nullptr;
        }
    }
}

int DisplayLayoutManager::count() const
{
    return m_rows * m_cols;
}
