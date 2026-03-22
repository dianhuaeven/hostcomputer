#include "RobotViewModel.h"

RobotViewModel::RobotViewModel(QObject *parent) : QObject(parent) {}

void RobotViewModel::setPitch(double v) { if (m_pitch != v) { m_pitch = v; emit pitchChanged(); } }
void RobotViewModel::setYaw(double v)   { if (m_yaw   != v) { m_yaw   = v; emit yawChanged();   } }
void RobotViewModel::setRoll(double v)  { if (m_roll  != v) { m_roll  = v; emit rollChanged();  } }

void RobotViewModel::setLeg1Angle(double v) { if (m_leg1 != v) { m_leg1 = v; emit leg1AngleChanged(); } }
void RobotViewModel::setLeg2Angle(double v) { if (m_leg2 != v) { m_leg2 = v; emit leg2AngleChanged(); } }
void RobotViewModel::setLeg3Angle(double v) { if (m_leg3 != v) { m_leg3 = v; emit leg3AngleChanged(); } }
void RobotViewModel::setLeg4Angle(double v) { if (m_leg4 != v) { m_leg4 = v; emit leg4AngleChanged(); } }

void RobotViewModel::updateAttitude(double roll, double pitch, double yaw)
{
    setRoll(roll);
    setPitch(pitch);
    setYaw(yaw);
}

void RobotViewModel::updateLegs(double l1, double l2, double l3, double l4)
{
    setLeg1Angle(l1);
    setLeg2Angle(l2);
    setLeg3Angle(l3);
    setLeg4Angle(l4);
}
