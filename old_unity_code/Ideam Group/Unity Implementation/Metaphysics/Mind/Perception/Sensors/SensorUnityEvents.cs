using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    public class SensorUnityEvents : MonoBehaviour
    {
        public string sensorType; 
        
        [SerializeField]
        protected BaseSensor sensor;
        public BaseSensor Sensor { get { return sensor; } set { if(value != sensor) UpdateSensorEvents(value); } }

        private void UpdateSensorEvents(BaseSensor newSensor)
        {
            if(sensor != null && Application.isPlaying) { ReleaseSensorEvents(); }

            sensor = newSensor;
            sensorType = newSensor.GetType().Name;
            int newLength = 0;

            if(sensorType == "AudioSensor")
            {
                newLength = 3;
            }
            else if (sensorType == "Collision2DSensor")
            {
                newLength = 5;
            }
            else if (sensorType == "CollisionSensor")
            {
                newLength = 5;
            }
            else if (sensorType == "JointSensor")
            {
                newLength = 1;
            }
            else if (sensorType == "RaySensor")
            {
                RaySensor rs = (RaySensor)sensor;
                newLength = 7 * rs.rays.Length;
            }
            else if (sensorType == "Trigger2DSensor")
            {
                newLength = 5;
            }
            else if (sensorType == "TriggerSensor")
            {
                newLength = 5;
            }

            if(newLength != unityEvents.Length) { System.Array.Resize(ref unityEvents, newLength); OnSetSensor?.Invoke(sensor); }
        }

        public event Action<BaseSensor> OnSetSensor;
        public UnityEvent[] unityEvents = new UnityEvent[0];

        private void Awake()
        { AttachSensorEvents(); }

        private void AttachSensorEvents() 
        { 
            if (sensorType == "AudioSensor")
            {
                AudioSensor aud = (AudioSensor)sensor;
                aud.OnStartHear.sensorEvent += evt => { unityEvents[0].Invoke(); };
                aud.OnHear.sensorEvent += evt => { unityEvents[1].Invoke(); };
                aud.OnStopHear.sensorEvent += evt => { unityEvents[2].Invoke(); };
            }
            else if (sensorType == "Collision2DSensor")
            {
                Collision2DSensor c2d = (Collision2DSensor)sensor; 
                c2d.OnFirstCollision2DStart.sensorEvent += evt => { unityEvents[0].Invoke(); };
                c2d.OnCollision2DStart.sensorEvent += evt => { unityEvents[1].Invoke(); };
                c2d.While2DColliding.sensorEvent += evt => { unityEvents[2].Invoke(); };
                c2d.OnCollision2DStop.sensorEvent += evt => { unityEvents[3].Invoke(); };
                c2d.OnLastCollision2DStop.sensorEvent += evt => { unityEvents[4].Invoke(); }; 
            }
            else if (sensorType == "CollisionSensor")
            {
                CollisionSensor col = (CollisionSensor)sensor;
                col.OnFirstCollisionStart.sensorEvent += evt => { unityEvents[0].Invoke(); };
                col.OnCollisionStart.sensorEvent += evt => { unityEvents[1].Invoke(); };
                col.WhileColliding.sensorEvent += evt => { unityEvents[2].Invoke(); };
                col.OnCollisionStop.sensorEvent += evt => { unityEvents[3].Invoke(); };
                col.OnLastCollisionStop.sensorEvent += evt => { unityEvents[4].Invoke(); };
            }
            else if (sensorType == "JointSensor")
            {
                JointSensor joint = (JointSensor)sensor;

                joint.JointBreak.sensorEvent += evt => { unityEvents[0].Invoke(); };
            }
            else if (sensorType == "RaySensor")
            {
                RaySensor ray = (RaySensor)sensor;
                
                for(int i=0; i < ray.rays.Length; i++)
                {
                    SensorRay sr = ray.rays[i];
                    int ri = i * 7;
                    sr.OnStartCasting.sensorEvent += evt => { unityEvents[ri + 0].Invoke(); };
                    sr.WhileCasting.sensorEvent += evt => { unityEvents[ri + 1].Invoke(); };
                    sr.OnStopCasting.sensorEvent += evt => { unityEvents[ri + 2].Invoke(); };
                    sr.OnStartHitting.sensorEvent += evt => { unityEvents[ri + 3].Invoke(); };
                    sr.WhileHitting.sensorEvent += evt => { unityEvents[ri + 4].Invoke(); };
                    sr.OnStopHitting.sensorEvent += evt => { unityEvents[ri + 5].Invoke(); };
                    sr.OnNotHitting.sensorEvent += evt => { unityEvents[ri + 6].Invoke(); };
                }
            }
            else if (sensorType == "Trigger2DSensor")
            {
                Trigger2DSensor t2d = (Trigger2DSensor)sensor;
                t2d.OnFirstTrigger2DStart.sensorEvent += evt => { unityEvents[0].Invoke(); };
                t2d.OnTrigger2DStart.sensorEvent += evt => { unityEvents[1].Invoke(); };
                t2d.While2DTriggered.sensorEvent += evt => { unityEvents[2].Invoke(); };
                t2d.OnTrigger2DStop.sensorEvent += evt => { unityEvents[3].Invoke(); };
                t2d.OnLastTrigger2DStop.sensorEvent += evt => { unityEvents[4].Invoke(); };
            }
            else if (sensorType == "TriggerSensor")
            {
                TriggerSensor trigger = (TriggerSensor)sensor;
                trigger.OnFirstTriggerStart.sensorEvent += evt => { unityEvents[0].Invoke(); };
                trigger.OnTriggerStart.sensorEvent += evt => { unityEvents[1].Invoke(); };
                trigger.WhileTriggered.sensorEvent += evt => { unityEvents[2].Invoke(); };
                trigger.OnTriggerStop.sensorEvent += evt => { unityEvents[3].Invoke(); };
                trigger.OnLastTriggerStop.sensorEvent += evt => { unityEvents[4].Invoke(); };
            }
        }

        private void ReleaseSensorEvents()
        {
            if (sensorType == "AudioSensor")
            {
                AudioSensor aud = (AudioSensor)sensor;
                aud.OnStartHear.sensorEvent -= evt => { unityEvents[0].Invoke(); };
                aud.OnHear.sensorEvent -= evt => { unityEvents[1].Invoke(); };
                aud.OnStopHear.sensorEvent -= evt => { unityEvents[2].Invoke(); };
            }
            else if (sensorType == "Collision2DSensor")
            {
                Collision2DSensor c2d = (Collision2DSensor)sensor;
                c2d.OnFirstCollision2DStart.sensorEvent -= evt => { unityEvents[0].Invoke(); };
                c2d.OnCollision2DStart.sensorEvent -= evt => { unityEvents[1].Invoke(); };
                c2d.While2DColliding.sensorEvent -= evt => { unityEvents[2].Invoke(); };
                c2d.OnCollision2DStop.sensorEvent -= evt => { unityEvents[3].Invoke(); };
                c2d.OnLastCollision2DStop.sensorEvent -= evt => { unityEvents[4].Invoke(); };
            }
            else if (sensorType == "CollisionSensor")
            {
                CollisionSensor col = (CollisionSensor)sensor;
                col.OnFirstCollisionStart.sensorEvent -= evt => { unityEvents[0].Invoke(); };
                col.OnCollisionStart.sensorEvent -= evt => { unityEvents[1].Invoke(); };
                col.WhileColliding.sensorEvent -= evt => { unityEvents[2].Invoke(); };
                col.OnCollisionStop.sensorEvent -= evt => { unityEvents[3].Invoke(); };
                col.OnLastCollisionStop.sensorEvent -= evt => { unityEvents[4].Invoke(); };
            }
            else if (sensorType == "JointSensor")
            {
                JointSensor joint = (JointSensor)sensor;

                joint.JointBreak.sensorEvent -= evt => { unityEvents[0].Invoke(); };
            }
            else if (sensorType == "RaySensor")
            {
                RaySensor ray = (RaySensor)sensor;

                for (int i = 0; i < ray.rays.Length; i++)
                {
                    SensorRay sr = ray.rays[i];
                    int ri = i * 7;
                    sr.OnStartCasting.sensorEvent -= evt => { unityEvents[ri + 0].Invoke(); };
                    sr.WhileCasting.sensorEvent -= evt => { unityEvents[ri + 1].Invoke(); };
                    sr.OnStopCasting.sensorEvent -= evt => { unityEvents[ri + 2].Invoke(); };
                    sr.OnStartHitting.sensorEvent -= evt => { unityEvents[ri + 3].Invoke(); };
                    sr.WhileHitting.sensorEvent -= evt => { unityEvents[ri + 4].Invoke(); };
                    sr.OnStopHitting.sensorEvent -= evt => { unityEvents[ri + 5].Invoke(); };
                    sr.OnNotHitting.sensorEvent -= evt => { unityEvents[ri + 6].Invoke(); };
                }
            }
            else if (sensorType == "Trigger2DSensor")
            {
                Trigger2DSensor t2d = (Trigger2DSensor)sensor;
                t2d.OnFirstTrigger2DStart.sensorEvent -= evt => { unityEvents[0].Invoke(); };
                t2d.OnTrigger2DStart.sensorEvent -= evt => { unityEvents[1].Invoke(); };
                t2d.While2DTriggered.sensorEvent -= evt => { unityEvents[2].Invoke(); };
                t2d.OnTrigger2DStop.sensorEvent -= evt => { unityEvents[3].Invoke(); };
                t2d.OnLastTrigger2DStop.sensorEvent -= evt => { unityEvents[4].Invoke(); };
            }
            else if (sensorType == "TriggerSensor")
            {
                TriggerSensor trigger = (TriggerSensor)sensor;
                trigger.OnFirstTriggerStart.sensorEvent -= evt => { unityEvents[0].Invoke(); };
                trigger.OnTriggerStart.sensorEvent -= evt => { unityEvents[1].Invoke(); };
                trigger.WhileTriggered.sensorEvent -= evt => { unityEvents[2].Invoke(); };
                trigger.OnTriggerStop.sensorEvent -= evt => { unityEvents[3].Invoke(); };
                trigger.OnLastTriggerStop.sensorEvent -= evt => { unityEvents[4].Invoke(); };
            }
        }


#if UNITY_EDITOR
        private void Reset()
        {
            BaseSensor bs = gameObject.GetComponent<BaseSensor>();
            if (bs) { Sensor = bs; }
        }
#endif
    }
}