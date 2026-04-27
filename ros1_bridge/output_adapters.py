from typing import Optional

from bridge_state import TwistCommand
from debug_events import EventSink, NullEventSink


class OutputAdapter:
    def __init__(self, events: Optional[EventSink] = None) -> None:
        self.events = events or NullEventSink()

    def publish_twist(self, twist: TwistCommand) -> None:
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


class RosOutput(OutputAdapter):
    def __init__(self, node_name: str, topic: str, events: Optional[EventSink] = None) -> None:
        super().__init__(events)
        import rospy
        from geometry_msgs.msg import Twist

        self._twist_type = Twist
        rospy.init_node(node_name, anonymous=False)
        self._publisher = rospy.Publisher(topic, Twist, queue_size=1)
        rospy.loginfo("host_bridge_node publishing Twist to %s", topic)

    def publish_twist(self, twist: TwistCommand) -> None:
        msg = self._twist_type()
        msg.linear.x = twist.linear_x
        msg.angular.z = twist.angular_z
        self._publisher.publish(msg)
        self.events.emit("output", "ros cmd_vel published", data={
            "linear_x": twist.linear_x,
            "angular_z": twist.angular_z,
            "source": twist.source,
        })
