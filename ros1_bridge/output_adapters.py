from typing import Optional

from bridge_state import GripperCommand, ServoCommand, TwistCommand
from debug_events import EventSink, NullEventSink


class OutputAdapter:
    def __init__(self, events: Optional[EventSink] = None) -> None:
        self.events = events or NullEventSink()

    def publish_twist(self, twist: TwistCommand) -> None:
        raise NotImplementedError

    def publish_servo(self, servo: ServoCommand) -> None:
        raise NotImplementedError

    def publish_gripper(self, gripper: GripperCommand) -> None:
        raise NotImplementedError


class DryRunOutput(OutputAdapter):
    def publish_twist(self, twist: TwistCommand) -> None:
        print(
            f"[bridge] cmd_vel source={twist.source} "
            f"linear_x={twist.linear_x:+.3f} angular_z={twist.angular_z:+.3f}",
            flush=True,
        )
        self.events.emit("output", "dry-run cmd_vel", data={
            "linear_x": twist.linear_x,
            "angular_z": twist.angular_z,
            "source": twist.source,
        })

    def publish_servo(self, servo: ServoCommand) -> None:
        print(
            f"[bridge] servo source={servo.source} frame={servo.frame_id} "
            f"lin=({servo.linear_x:+.3f},{servo.linear_y:+.3f},{servo.linear_z:+.3f}) "
            f"ang=({servo.angular_x:+.3f},{servo.angular_y:+.3f},{servo.angular_z:+.3f})",
            flush=True,
        )
        self.events.emit("output", "dry-run servo", data=servo.to_dict())

    def publish_gripper(self, gripper: GripperCommand) -> None:
        print(
            f"[bridge] gripper source={gripper.source} position={gripper.position:.4f}",
            flush=True,
        )
        self.events.emit("output", "dry-run gripper", data=gripper.to_dict())


class RosOutput(OutputAdapter):
    def __init__(
        self,
        node_name: str,
        cmd_vel_topic: str,
        servo_topic: str,
        gripper_position_topic: str,
        events: Optional[EventSink] = None,
    ) -> None:
        super().__init__(events)
        import rospy
        from geometry_msgs.msg import Twist, TwistStamped
        from std_msgs.msg import Float64

        self._twist_type = Twist
        self._twist_stamped_type = TwistStamped
        self._float64_type = Float64
        self._rospy = rospy
        rospy.init_node(node_name, anonymous=False)
        self._cmd_vel_pub = rospy.Publisher(cmd_vel_topic, Twist, queue_size=1)
        self._servo_pub = rospy.Publisher(servo_topic, TwistStamped, queue_size=1)
        self._gripper_pub = rospy.Publisher(gripper_position_topic, Float64, queue_size=1)
        rospy.loginfo("host_bridge_node publishing Twist to %s", cmd_vel_topic)
        rospy.loginfo("host_bridge_node publishing TwistStamped to %s", servo_topic)
        rospy.loginfo("host_bridge_node publishing Float64 to %s", gripper_position_topic)

    def publish_twist(self, twist: TwistCommand) -> None:
        msg = self._twist_type()
        msg.linear.x = twist.linear_x
        msg.angular.z = twist.angular_z
        self._cmd_vel_pub.publish(msg)
        self.events.emit("output", "ros cmd_vel published", data={
            "linear_x": twist.linear_x,
            "angular_z": twist.angular_z,
            "source": twist.source,
        })

    def publish_servo(self, servo: ServoCommand) -> None:
        msg = self._twist_stamped_type()
        msg.header.stamp = self._rospy.Time.now()
        msg.header.frame_id = servo.frame_id
        msg.twist.linear.x = servo.linear_x
        msg.twist.linear.y = servo.linear_y
        msg.twist.linear.z = servo.linear_z
        msg.twist.angular.x = servo.angular_x
        msg.twist.angular.y = servo.angular_y
        msg.twist.angular.z = servo.angular_z
        self._servo_pub.publish(msg)
        self.events.emit("output", "ros servo published", data=servo.to_dict())

    def publish_gripper(self, gripper: GripperCommand) -> None:
        msg = self._float64_type(data=gripper.position)
        self._gripper_pub.publish(msg)
        self.events.emit("output", "ros gripper published", data=gripper.to_dict())
