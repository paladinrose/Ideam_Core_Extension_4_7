
using System;
using UnityEngine;
using UnityEngine.Events;
using System.Collections;
using System.Collections.Generic;

#if UNITY_EDITOR
using UnityEditor;
#endif
namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    [AddComponentMenu("Ideam/Sensors/Joint Sensor")]
    public class JointSensor : BaseSensor
    {

        public JointSensorEvent JointBreak;

        protected Joint joint;
        private void Awake()
        {
            joint = GetComponent<Joint>();
        }

        private void OnJointBreak(float breakForce)
        {
            if (!IsSensing) return;

            JointBreak.Invoke(gameObject, new JointSensorData(this, joint, breakForce));
        }

        public void SetJoint(Joint newJoint)
        {
            if (joint)
            {

            }
            joint = newJoint;
        }
    }

    [System.Serializable]
    public struct JointSensorData
    {
        public JointSensor sensor;
        public Joint joint;
        public float breakForce;

        public JointSensorData(JointSensor sensor, Joint joint, float breakForce)
        {
            this.sensor = sensor;
            this.joint = joint;
            this.breakForce = breakForce;
        }
    }

    public delegate void JointSensorDataHandler(JointSensorData sensorData);

    [System.Serializable]
    public class JointSensorEvent : SensorEvent<JointSensorData>
    {

        public event JointSensorDataHandler sensorEvent;

        public override bool Invoke(GameObject targetObject, JointSensorData eventData)
        {
            if (base.Invoke(targetObject, eventData))
            {
                sensorEvent?.Invoke(eventData);
                return true;
            }
            return false;
        }
    }
}
