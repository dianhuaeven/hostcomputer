#ifndef CO2DISPLAYWIDGET_H
#define CO2DISPLAYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class CO2DisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CO2DisplayWidget(QWidget *parent = nullptr);

    void setCO2Value(float ppm);

private:
    QLabel *m_titleLabel;
    QLabel *m_valueLabel;
    QLabel *m_statusLabel;

    void updateStatusColor(float ppm);
};

#endif // CO2DISPLAYWIDGET_H
