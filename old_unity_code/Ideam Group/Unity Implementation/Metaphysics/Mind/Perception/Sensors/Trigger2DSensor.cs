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
    [AddComponentMenu("Ideam/Sensors/Trigger2D Sensor")]
    public class Trigger2DSensor : BaseSensor
    {

        public Trigger2DSensorEvent OnFirstTrigger2DStart, OnTrigger2DStart, While2DTriggered, OnTrigger2DStop, OnLastTrigger2DStop;
        public List<Collider2D> collider2DsInTrigger = new List<Collider2D>();
        public bool acceptTriggers = false;

        public GameObject latestToEnter
        {
            get
            {
                int c = collider2DsInTrigger.Count;
                if (c > 0)
                {
                    return collider2DsInTrigger[c - 1].gameObject;
                }
                else
                {
                    return null;
                }
            }
        }


        void OnTriggerEnter2D(Collider2D col)
        {
            if (!acceptTriggers && col.isTrigger) { return; }
            bool first = (collider2DsInTrigger.Count == 0);
            collider2DsInTrigger.Add(col);
            if (IsSensing)
            {
                if (first) { OnFirstTrigger2DStart.Invoke(col.gameObject, new Trigger2DSensorData(this, 0)); }

                OnTrigger2DStart.Invoke(col.gameObject, new Trigger2DSensorData(this, collider2DsInTrigger.Count-1));

            }
        }

        void OnTriggerStay2D(Collider2D col)
        {
            if (!acceptTriggers && col.isTrigger) { return; }
            if (IsSensing)
            {
                int cid = collider2DsInTrigger.IndexOf(col);
                if(cid >= 0)
                {
                    While2DTriggered.Invoke(col.gameObject, new Trigger2DSensorData(this, cid)); 
                }
            }
            
        }

        void OnTriggerExit2D(Collider2D col)
        {
            if (!acceptTriggers && col.isTrigger) { return; }
            if (IsSensing)
            {
                int cid = collider2DsInTrigger.IndexOf(col);
                if(cid < 0) { return; }

                OnTrigger2DStop.Invoke(col.gameObject, new Trigger2DSensorData(this, cid));
                if (collider2DsInTrigger.Count == 1 && collider2DsInTrigger[0] == col)
                {
                    OnLastTrigger2DStop.Invoke(col.gameObject, new Trigger2DSensorData(this, cid));
                }
            }
            collider2DsInTrigger.Remove(col);
            
        }

        public GameObject[] GetAllWithin(string tag, bool includChildren = false)
        {
            List<GameObject> obs = new List<GameObject>();
            for(int i = 0; i < collider2DsInTrigger.Count; i++)
            {
                Transform[] alls = collider2DsInTrigger[i].GetComponentsInChildren<Transform>();
                for(int j = 0; j < alls.Length; j++)
                {
                    if (alls[i].tag == tag)
                    {
                        obs.Add(alls[i].gameObject);
                    }
                }
            }
            return obs.ToArray();
        }

        public T[] GetAllWithin<T>(bool includeChildren = false)
        {
            List<T> comps = new List<T>();
            for (int i = 0; i < collider2DsInTrigger.Count; i++)
            {
                T[] c;
                if (!includeChildren)
                {
                    c = collider2DsInTrigger[i].GetComponents<T>();
                }
                else
                {
                    c = collider2DsInTrigger[i].GetComponentsInChildren<T>();
                }
                comps.AddRange(c);
            }
            return comps.ToArray();
        }
        

    }

    [System.Serializable]
    public struct Trigger2DSensorData
    {
        public Trigger2DSensor sensor;
        public int colliderID;
        public Collider2D collider2D;

        public Trigger2DSensorData(Trigger2DSensor s, int id)
        {
            sensor = s;
            colliderID = id;
            collider2D = sensor.collider2DsInTrigger[id];
        }
    }
    public delegate void Trigger2DSensorDataHandler(Trigger2DSensorData sensorData);

    [System.Serializable]
    public class Trigger2DSensorEvent : SensorEvent<Trigger2DSensorData>
    {

        public event Trigger2DSensorDataHandler sensorEvent;
        public event System.Action<GameObject> hitEvent;

        public override bool Invoke(GameObject targetObject, Trigger2DSensorData eventData)
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
