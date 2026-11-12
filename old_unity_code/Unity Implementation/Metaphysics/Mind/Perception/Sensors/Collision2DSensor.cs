using UnityEngine;
using UnityEngine.Events;
using System.Collections;
using System.Collections.Generic;
using System;
#if UNITY_EDITOR
using UnityEditor;
#endif
namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    [AddComponentMenu("Ideam/Sensors/Collision2D Sensor")]
    public class Collision2DSensor : BaseSensor
    {
        public Collision2DSensorEvent OnFirstCollision2DStart, OnCollision2DStart, While2DColliding, OnCollision2DStop, OnLastCollision2DStop;

        public List<Collider2D> collider2DsInContact = new List<Collider2D>();

        public GameObject latestToEnter
        {
            get
            {
                int c = collider2DsInContact.Count;
                if (c > 0)
                {
                    return collider2DsInContact[c - 1].gameObject;
                }
                else
                {
                    return null;
                }
            }
        }

        public int GetCollider2DID(Collision2D col)
        {
            for (int i = collider2DsInContact.Count - 1; i >= 0; i--)
            {
                if (collider2DsInContact[i] == col.collider)
                {
                    return i;
                }
            }
            return -1;
        }

        void OnCollisionEnter2D(Collision2D col)
        {
            bool first = (collider2DsInContact.Count == 0);
            collider2DsInContact.Add(col.collider);
            if (IsSensing)
            {
                if (first) { OnFirstCollision2DStart.Invoke(col.gameObject, new Collision2DSensorData(this, 0, col)); }

                OnCollision2DStart.Invoke(col.gameObject, new Collision2DSensorData(this, collider2DsInContact.Count-1, col));

            }
        }

        void OnCollisionStay2D(Collision2D col)
        {
            if (IsSensing)
            {
                int cid = GetCollider2DID(col);
                if(cid < 0) { return; }
                While2DColliding.Invoke(col.gameObject, new Collision2DSensorData(this, cid,col));
            }
        }

        void OnCollisionExit2D(Collision2D col)
        {
            if (IsSensing)
            {
                int cid = GetCollider2DID(col);
                OnCollision2DStop.Invoke(col.gameObject, new Collision2DSensorData(this, cid, col));
                if (collider2DsInContact.Count == 1 && collider2DsInContact[0] == col.collider)
                {
                    OnLastCollision2DStop.Invoke(col.gameObject, new Collision2DSensorData(this, cid, col));
                }
            }
            collider2DsInContact.Remove(col.collider);
        }
        
        public T[] GetAllWithin<T>(bool includeChildren = false)
        {
            List<T> comps = new List<T>();
            for (int i = 0; i < collider2DsInContact.Count; i++)
            {
                T[] c;
                if (!includeChildren)
                {
                    c = collider2DsInContact[i].GetComponents<T>();
                }
                else
                {
                    c = collider2DsInContact[i].GetComponentsInChildren<T>();
                }
                comps.AddRange(c);
            }
            return comps.ToArray();
        }


    }
    [System.Serializable]
    public struct Collision2DSensorData
    {
        public Collision2DSensor sensor;
        public int colliderID;
        public Collision2D collision2D;

        public Collision2DSensorData(Collision2DSensor s, int id, Collision2D c)
        {
            sensor = s;
            colliderID = id;
            collision2D = c;
        }
    }

    public delegate void Collision2DSensorDataHandler(Collision2DSensorData sensorData);
    [System.Serializable]
    public class Collision2DSensorEvent : SensorEvent<Collision2DSensorData>
    {
        public event Collision2DSensorDataHandler sensorEvent;

        public override bool Invoke(GameObject targetObject, Collision2DSensorData eventData)
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
