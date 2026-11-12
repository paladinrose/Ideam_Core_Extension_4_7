
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
    [AddComponentMenu("Ideam/Sensors/Trigger Sensor")]
    public class TriggerSensor : BaseSensor
    {
        
        public TriggerSensorEvent OnFirstTriggerStart, OnTriggerStart, WhileTriggered, OnTriggerStop, OnLastTriggerStop;

        public List<Collider> collidersInTrigger = new List<Collider>();

        public GameObject LatestToEnter
        {
            get
            {
                int c = collidersInTrigger.Count;
                if (c > 0)
                {
                    return collidersInTrigger[c - 1].gameObject;
                }
                return null;
            }
        }

        void OnTriggerEnter(Collider col)
        {
            bool first = (collidersInTrigger.Count == 0);
            collidersInTrigger.Add(col);
            if (IsSensing)
            {
                if (first) { OnFirstTriggerStart.Invoke(col.gameObject, new TriggerSensorData(this, 0)); }
                
                OnTriggerStart.Invoke(col.gameObject, new TriggerSensorData(this, collidersInTrigger.Count-1));
               
            }
        }

        void OnTriggerStay(Collider col)
        {
            if (IsSensing)
            {
                int cid = collidersInTrigger.IndexOf(col);
                
                if(cid>=0)
                {
                    WhileTriggered.Invoke(col.gameObject, new TriggerSensorData(this, cid));
                }
                
            } 
        }

        void OnTriggerExit(Collider col)
        {
            if (IsSensing)
            {
                int cid = collidersInTrigger.IndexOf(col);
                if(cid < 0) { return; }

                OnTriggerStop.Invoke(col.gameObject, new TriggerSensorData(this, cid));
                if (collidersInTrigger.Count == 1 && collidersInTrigger[0] == col)
                {
                    OnLastTriggerStop.Invoke(col.gameObject, new TriggerSensorData(this, cid));
                }
                collidersInTrigger.RemoveAt(cid);
            }
        }

        public T[] GetAllWithin<T>(bool includeChildren = false)
        {
            List<T> comps = new List<T>();
            for (int i = 0; i < collidersInTrigger.Count; i++)
            {
                T[] c;
                if (!includeChildren)
                {
                    c = collidersInTrigger[i].GetComponents<T>();
                }
                else
                {
                    c = collidersInTrigger[i].GetComponentsInChildren<T>();
                }
                comps.AddRange(c);
            }
            return comps.ToArray();
        }
        
    }
    
    [System.Serializable]
    public struct TriggerSensorData
    {
        public TriggerSensor sensor;
        public int colliderID;
        public Collider collider;

        public TriggerSensorData(TriggerSensor s, int id)
        {
            sensor = s;
            colliderID = id;
            collider = sensor.collidersInTrigger[id];
        }
    }

    public delegate void TriggerSensorDataHandler(TriggerSensorData sensorData);

    [System.Serializable]
    public class TriggerSensorEvent : SensorEvent<TriggerSensorData>
    {

        public event TriggerSensorDataHandler sensorEvent;
        public event System.Action<GameObject> hitEvent;

        public override bool Invoke(GameObject targetObject, TriggerSensorData eventData)
        {
            
            if (base.Invoke(targetObject, eventData))
            {
                sensorEvent?.Invoke(eventData);
                hitEvent?.Invoke(targetObject);
                return true;
            }
            return false;
        }
    }
}


