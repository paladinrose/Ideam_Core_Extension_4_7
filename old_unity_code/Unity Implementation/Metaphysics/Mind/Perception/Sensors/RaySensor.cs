using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.Events;
using System;


#if UNITY_EDITOR
using UnityEditor;
#endif

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    [AddComponentMenu("Ideam/Sensors/Ray Sensor")]
    public class RaySensor : BaseSensor
    {
        public SensorRay[] rays = new SensorRay[0];

       
        private void Start()
        {
            if (!IsSensing) { return; }
            for (int i = 0; i < rays.Length; i++)
            {
                if (rays[i].castOnStart)
                {
                    StartCasting(i);
                }
            }
        }

        public void CastRay(int rayID)
        {
            SensorRay sr = rays[rayID];
           
            Ray ray = sr.GetRay();
            RaySensorData data = new RaySensorData { ray = ray, rayCast = this, rayID = rayID, duration = 0f };
            RaycastHit hit;

            if (UnityEngine.Physics.Raycast(ray, out hit, sr.length, sr.layerMask.value))
            {

                sr.lastHit = hit;
                DetectableVisual otherVisual = hit.collider.GetComponentInChildren<DetectableVisual>();

                data.hit = hit;
                
                sr.OnStartHitting?.Invoke(gameObject, data);

                if (otherVisual)
                {
                    sr.lastHitVisual = otherVisual;
                    otherVisual.OnSeen?.Invoke(gameObject, data);

                }
            }
            else
            {
                sr.OnNotHitting?.Invoke(gameObject, data);
            }
        }

        public void StartCasting(int rayID)
        {
            if (!IsSensing) { return; }
            
            if (!rays[rayID].isCasting)
            {
                
                SensorRay sr = rays[rayID];
                sr.currentTime = sr.currentDuration = sr.hitDuration = sr.seenDuration = 0f;

                Ray ray = sr.GetRay();
                RaySensorData data = new RaySensorData { ray = ray, rayCast = this, rayID = rayID, duration = 0f};
                RaycastHit hit;

                if (UnityEngine.Physics.Raycast(ray, out hit, sr.length,sr.layerMask.value))
                {

                    sr.lastHit = hit;
                    DetectableVisual otherVisual = hit.collider.GetComponentInChildren<DetectableVisual>();
                    
                    data.hit = hit;
                    sr.OnStartHitting?.Invoke(gameObject, data);
                    
                    if (otherVisual)
                    {
                       
                        sr.lastHitVisual = otherVisual;
                        otherVisual.OnSeen?.Invoke(gameObject, data);
                        
                    }
                }

                data.hit = hit;
                sr.isCasting = true;
                sr.OnStartCasting?.Invoke(gameObject, data);
                
            }
        }
        public void StartCastingAll()
        {
            for(int i = 0; i < rays.Length; i++)
            {
                StartCasting(i);
            }
        }

        public void StopCasting(int rayID)
        {
            if (rays[rayID].isCasting)
            {
                SensorRay sr = rays[rayID];

                RaySensorData data = new RaySensorData
                {
                    rayCast = this,
                    rayID = rayID,
                    hit = sr.lastHit,
                    rayHit = sr.lastHitVisual,
                    duration = sr.seenDuration
                };

                if (sr.lastHitVisual)
                {
                    sr.lastHitVisual.OnStoppedBeingSeen?.Invoke(gameObject, data);

                }

                data.duration = sr.hitDuration;

                if (sr.lastHit.collider != null)
                {
                    sr.OnStopHitting?.Invoke(gameObject, data);

                    data.hit = new RaycastHit();
                    data.duration = 0f;
                    data.rayHit = null;
                    sr.OnNotHitting?.Invoke(gameObject, data);
                }

                data.rayHit = null;
                sr.lastHitVisual = null;
                sr.lastHit = data.hit = new RaycastHit();
                data.duration = sr.currentDuration;
                sr.OnStopCasting?.Invoke(gameObject, data);
                sr.isCasting = false;
            }
        }
        public void StopCastingAll()
        {
            for(int i = 0; i < rays.Length; i++)
            {
                StopCasting(i);
            }
        }

        private void FixedUpdate()
        {

            if (!IsSensing) { return; }
            float delta = Time.fixedDeltaTime;
            for (int i = 0; i < rays.Length; i++)
            {
                SensorRay sr = rays[i];

                if (!sr.isCasting) { continue; }

                sr.currentDuration += delta;
                sr.currentTime += delta;
                if (sr.currentTime < sr.recastTime) { continue; }
                sr.currentTime -= sr.recastTime;

                Ray ray = sr.GetRay();
                RaycastHit hit;
                RaySensorData data = new RaySensorData { rayCast = this, ray = ray, rayID = i };

                if (UnityEngine.Physics.Raycast(ray, out hit, sr.length,sr.layerMask.value))
                {
                    DetectableVisual otherVisual = hit.collider.GetComponentInChildren<DetectableVisual>();
                    sr.isHitting = true;
                    if (sr.lastHit.collider != null)
                    {
                        if(sr.lastHit.collider == hit.collider)
                        {
                            
                            sr.hitDuration += delta;
                            data.hit = hit;
                            data.duration = sr.hitDuration;
                            sr.WhileHitting?.Invoke(gameObject, data);
                        }
                        else
                        {
                            
                            data.hit = sr.lastHit;
                            data.duration = sr.hitDuration;
                            sr.OnStopHitting?.Invoke(gameObject, data);

                            data.hit = hit;
                            sr.hitDuration = data.duration = 0f;
                            sr.OnStartHitting?.Invoke(gameObject, data);
                            
                        }
                    }
                    else
                    {
                        data.hit = hit;
                        sr.hitDuration = data.duration = 0f;
                        sr.OnStartHitting?.Invoke(gameObject, data);
                    }

                    if (otherVisual)
                    {
                        if(sr.lastHitVisual == otherVisual)
                        {
                            sr.seenDuration += delta;
                            data.rayHit = otherVisual;
                            data.duration = sr.seenDuration;
                            otherVisual.WhileSeen?.Invoke(gameObject, data);
                        }
                        else
                        {
                            if (sr.lastHitVisual)
                            {
                                data.rayHit = sr.lastHitVisual;
                                data.hit = sr.lastHit;
                                data.duration = sr.seenDuration;
                                sr.lastHitVisual.OnStoppedBeingSeen?.Invoke(gameObject, data);
                            }

                            data.rayHit = otherVisual;
                            data.hit = hit;
                            data.duration = sr.seenDuration = 0f;
                            sr.lastHitVisual = otherVisual;
                            otherVisual.OnSeen?.Invoke(gameObject, data);
                        }
                    }
                }
                else
                {
                    

                    if (sr.lastHit.collider != null)
                    {
                        data.hit = sr.lastHit;
                        data.duration = sr.hitDuration;
                        sr.OnStopHitting?.Invoke(gameObject, data);

                        data.hit = hit;
                        sr.OnNotHitting?.Invoke(gameObject, data);
                    }

                    if (sr.lastHitVisual)
                    {
                        data.rayHit = sr.lastHitVisual;
                        data.hit = sr.lastHit;
                        data.duration = sr.seenDuration;

                        sr.lastHitVisual.OnStoppedBeingSeen?.Invoke(gameObject, data);
                        data.rayHit = null;
                        data.hit = hit;
                        sr.seenDuration = sr.hitDuration = 0f;
                        sr.lastHitVisual = null;
                    }
                    sr.isHitting = false;
                }
                sr.lastHit = hit;
                
                if (sr.currentDuration >= sr.duration)
                {
                    
                    StopCasting(i);
                }
                else
                {
                    data.duration = sr.currentDuration;
                    sr.WhileCasting?.Invoke(gameObject, data);
                }
            }
        }

        public override bool IsSensing
        {
            get { return isSensing; }
            set
            {
                if (value != isSensing)
                {
                    base.IsSensing = value;
                    if (!value)
                    {
                        StopCastingAll();
                    }
                    else
                    {
                       
                        for (int i = 0; i < rays.Length; i++)
                        {
                            if (rays[i].castOnActive)
                            {
                                StartCasting(i);
                            }
                        }
                    }
                }
            }
        }

        public void OnDrawGizmosSelected()
        {
            
            if (rays.Length == 0) { return; }
            if (IsSensing) { Gizmos.color = Color.green; } else
            {
                Gizmos.color = Color.gray;
            }

            for (int i = rays.Length - 1; i >= 0; i--)
            {
                Ray r = rays[i].GetRay();
                Gizmos.DrawLine(r.origin, r.origin + r.direction*rays[i].length);
            }
        }
    }

    [System.Serializable]
    public class SensorRay
    {
        public string name = "Sensor Ray";
        public float length = 1f, duration = float.PositiveInfinity, 
            currentDuration, recastTime, 
            currentTime, hitDuration, seenDuration;

        public bool castOnStart, castOnActive, scaleLengthToOriginTransform, isCasting, isHitting;

        public Vector3 origin, direction = Vector3.forward;
        public Space originSpace, directionSpace;
        public Transform originTransform;

        public RaySensorEvent OnStartCasting, WhileCasting, OnStopCasting,
                              OnStartHitting, WhileHitting, OnStopHitting,
                              OnNotHitting;

        public RaycastHit lastHit;
        public DetectableVisual lastHitVisual;
        public LayerMask layerMask;
        public Ray GetRay()
        {
            Vector3 o = origin, d = direction;
            if (originTransform)
            {
                if (originSpace == Space.Self)
                {
                    o = originTransform.TransformPoint(o);
                }
                if (directionSpace == Space.Self)
                {
                    d = originTransform.TransformDirection(d);
                }

            }
            return new Ray(o, d);
        }
        public Vector3 RayEnd
        {
            get
            {
                Vector3 rayEnd = GetRay().GetPoint(length);
                return rayEnd;
            }
        }
    }

    [System.Serializable]
    public struct RaySensorData
    {
        public RaySensor rayCast;
        public DetectableVisual rayHit;
        public int rayID;
        public Ray ray;
        public RaycastHit hit;
        public float duration;
        public RaySensorData(RaySensor caster, Ray r, int id, DetectableVisual hitter = null, RaycastHit h = default(RaycastHit))
        {
            rayCast = caster;
            rayHit = hitter;
            ray = r;
            rayID = id;
            hit = h;
            duration = 0;
        }

    }

    public delegate void RaySensorDataHandler(RaySensorData sensorData);

    [System.Serializable]
    public class RaySensorEvent : SensorEvent<RaySensorData>
    {
        public event RaySensorDataHandler sensorEvent;

        public override bool Invoke(GameObject targetObject, RaySensorData eventData)
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