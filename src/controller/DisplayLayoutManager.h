#ifndef DISPLAYLAYOUTMANAGER_H
#define DISPLAYLAYOUTMANAGER_H

#include <QWidget>
#include <QGridLayout>
#include <array>

class DisplayLayoutManager : public QWidget
{
    Q_OBJECT

public:
    explicit DisplayLayoutManager(int rows = 2, int cols = 3, QWidget *parent = nullptr);
    ~DisplayLayoutManager() override;

    // 在指定索引位置放入 widget（index 按行优先：0=r0c0, 1=r0c1, ...）
    void setWidget(int index, QWidget *widget);

    // 移除指定位置的 widget（不 delete，归还所有权给调用者）
    QWidget *removeWidget(int index);

    // 获取指定位置的 widget（nullptr 表示空位）
    QWidget *widget(int index) const;

    // 清空所有格子（不 delete widget）
    void clearAll();

    // 总格子数
    int count() const;

    // 行列数
    int rows() const { return m_rows; }
    int cols() const { return m_cols; }

private:
    int m_rows;
    int m_cols;
    QGridLayout *m_gridLayout;
    std::vector<QWidget*> m_widgets;  // 按 index 存储

    // index -> (row, col)
    int toRow(int index) const { return index / m_cols; }
    int toCol(int index) const { return index % m_cols; }
    bool isValidIndex(int index) const { return index >= 0 && index < count(); }
};

#endif // DISPLAYLAYOUTMANAGER_H
