#ifndef ROBOTVIEWMODEL_H
#define ROBOTVIEWMODEL_H

#include <QObject>

/**
 * @brief 机器人3D视图数据模型
 * 暴露给QML，用于驱动3D姿态可视化
 */
class RobotViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double pitch READ pitch WRITE setPitch NOTIFY pitchChanged)
    Q_PROPERTY(double yaw   READ yaw   WRITE setYaw   NOTIFY yawChanged)
    Q_PROPERTY(double roll  READ roll  WRITE setRoll  NOTIFY rollChanged)

    Q_PROPERTY(double leg1Angle READ leg1Angle WRITE setLeg1Angle NOTIFY leg1AngleChanged)
    Q_PROPERTY(double leg2Angle READ leg2Angle WRITE setLeg2Angle NOTIFY leg2AngleChanged)
    Q_PROPERTY(double leg3Angle READ leg3Angle WRITE setLeg3Angle NOTIFY leg3AngleChanged)
    Q_PROPERTY(double leg4Angle READ leg4Angle WRITE setLeg4Angle NOTIFY leg4AngleChanged)

public:
    explicit RobotViewModel(QObject *parent = nullptr);

    double pitch() const { return m_pitch; }
    double yaw()   const { return m_yaw;   }
    double roll()  const { return m_roll;  }

    double leg1Angle() const { return m_leg1; }
    double leg2Angle() const { return m_leg2; }
    double leg3Angle() const { return m_leg3; }
    double leg4Angle() const { return m_leg4; }

    void setPitch(double v);
    void setYaw(double v);
    void setRoll(double v);

    void setLeg1Angle(double v);
    void setLeg2Angle(double v);
    void setLeg3Angle(double v);
    void setLeg4Angle(double v);

    // 批量更新IMU数据
    Q_INVOKABLE void updateAttitude(double roll, double pitch, double yaw);
    // 批量更新腿部角度（关节位置，单位：原始值，自动换算为度）
    Q_INVOKABLE void updateLegs(double l1, double l2, double l3, double l4);

signals:
    void pitchChanged();
    void yawChanged();
    void rollChanged();
    void leg1AngleChanged();
    void leg2AngleChanged();
    void leg3AngleChanged();
    void leg4AngleChanged();

private:
    double m_pitch = 0.0;
    double m_yaw   = 0.0;
    double m_roll  = 0.0;
    double m_leg1  = 0.0;
    double m_leg2  = 0.0;
    double m_leg3  = 0.0;
    double m_leg4  = 0.0;
};

#endif // ROBOTVIEWMODEL_H
