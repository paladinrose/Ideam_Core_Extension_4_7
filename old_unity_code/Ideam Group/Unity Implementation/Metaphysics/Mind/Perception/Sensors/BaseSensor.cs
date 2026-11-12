using UnityEngine;
using UnityEngine.Events;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    
    public abstract class BaseSensor : MonoBehaviour
    {
        [SerializeField]
        protected bool isSensing;
        public virtual bool IsSensing {
            get {
                return isSensing;
            }
            set
            {
                if(isSensing != value)
                {
                    isSensing = value;
                    if (isSensing)
                    {
                        OnStartSensing?.Invoke(this);
                    }else
                    {
                        OnStopSensing?.Invoke(this);
                    }
                }
            }
        }
        public event BaseSensorHandler OnStartSensing, OnStopSensing;

    }

    public delegate void BaseSensorHandler(BaseSensor sensor);

    [System.Serializable]
    public class EventMask
    {
        
        public string[] tagMask = new string[0];
        public LayerMask layerMask = ~0;

        public bool CheckEventMask(GameObject gameObject)
        {
            if(CheckLayer(gameObject.layer) && CheckTag(gameObject.tag))
            {
                return true; 
            }

            return false;
        }

       

        public bool CheckLayer(int layer)
        {
            return layerMask == (layerMask | (1 << layer));
        }

        public bool CheckTag(string tag)
        {
            if (tagMask.Length == 0)
            {
                return true;
            }
            else
            {
                for (int i = tagMask.Length - 1; i >= 0; i--)
                {
                    if (tagMask[i] == tag)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
    }

    [System.Serializable]
    public abstract class SensorEvent <T> 
    {
        public EventMask eventMask;
        
        public virtual bool Invoke(GameObject targetObject, T eventData)
        {
            return eventMask.CheckEventMask(targetObject);
        }
    }
    
}