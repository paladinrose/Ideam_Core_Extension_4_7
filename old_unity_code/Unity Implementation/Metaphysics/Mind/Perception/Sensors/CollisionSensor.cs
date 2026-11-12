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
    [AddComponentMenu("Ideam/Sensors/Collision Sensor")]
    public class CollisionSensor : BaseSensor
    {
        public CollisionSensorEvent OnFirstCollisionStart, OnCollisionStart, WhileColliding, OnCollisionStop, OnLastCollisionStop;

        public Collider targetCollider;

        public List<Collider> collidersInContact = new List<Collider>();
        public List<float> durationsOfContact = new List<float>();

        public GameObject latestToEnter
        {
            get
            {
                int c = collidersInContact.Count;
                if (c > 0)
                {
                    return collidersInContact[c - 1].gameObject;
                }
                else
                {
                    return null;
                }
            }
        }

        public int GetColliderID(Collision col)
        {
            for(int i = collidersInContact.Count-1; i >=0; i--)
            {
                if(collidersInContact[i] == col.collider)
                {
                    return i;
                }
            }
            return -1;
        }

        void OnCollisionEnter(Collision col)
        {
            bool first = (collidersInContact.Count == 0);
            collidersInContact.Add(col.collider);
            durationsOfContact.Add(0);
            if (IsSensing)
            {
                if (first) { OnFirstCollisionStart.Invoke(col.gameObject, new CollisionSensorData(this, 0, col, 0)); }

                OnCollisionStart.Invoke(col.gameObject, new CollisionSensorData(this,collidersInContact.Count-1, col, 0));

            }
        }

        void OnCollisionStay(Collision col)
        {
            if(IsSensing)
            {
                int cid = GetColliderID(col);
                if(cid < 0) { return; }
                
                WhileColliding.Invoke(col.gameObject, new CollisionSensorData(this, cid,col, durationsOfContact[cid]));
            }
        }

        void OnCollisionExit(Collision col)
        {
            int cid = GetColliderID(col);
            if(cid < 0) { return; }
            if (IsSensing )
            {
                
                OnCollisionStop.Invoke(col.gameObject, new CollisionSensorData(this, cid,col, durationsOfContact[cid]));

                if (collidersInContact.Count == 1 && collidersInContact[0] == col.collider)
                {
                    OnLastCollisionStop.Invoke(col.gameObject, new CollisionSensorData(this, cid, col, durationsOfContact[cid]));
                }
            }
            collidersInContact.RemoveAt(cid);
            durationsOfContact.RemoveAt(cid);
        }
        
        public T[] GetAllWithin<T>(bool includeChildren = false)
        {
            List<T> comps = new List<T>();
            for (int i = 0; i < collidersInContact.Count; i++)
            {
                T[] c;
                if (!includeChildren)
                {
                    c = collidersInContact[i].GetComponents<T>();
                }
                else
                {
                    c = collidersInContact[i].GetComponentsInChildren<T>();
                }
                comps.AddRange(c);
            }
            return comps.ToArray();
        }

        //Ok, within the FixedUpdate, I need to fix how the detectionLimit works. Right now, I'm just using unlimited anywho.
        public BoxDetector[] boxDetectors = new BoxDetector[0];
        public CapsuleDetector[] capsuleDetectors = new CapsuleDetector[0];
        public SphereDetector[] sphereDetectors = new SphereDetector[0];

        public int GetDetectorID(string dName, DetectorType type)
        {
            switch (type)
            {
                case DetectorType.Box:

                    for (int i = boxDetectors.Length - 1; i >= 0; i--)
                    {
                        if (boxDetectors[i].name == dName)
                        {
                            return i;
                        }
                    }

                    break;

                case DetectorType.Capsule:

                    for (int i = capsuleDetectors.Length - 1; i >= 0; i--)
                    {
                        if (capsuleDetectors[i].name == dName)
                        {
                            return i;
                        }
                    }

                    break;

                case DetectorType.Sphere:

                    for (int i = sphereDetectors.Length - 1; i >= 0; i--)
                    {
                        if (sphereDetectors[i].name == dName)
                        {
                            return i;
                        }
                    }

                    break;
            }
            

            return -1;
        }

        private void FixedUpdate()
        {

            float fdt = Time.fixedDeltaTime;

            for(int i = durationsOfContact.Count-1; i >=0; i--)
            {
                durationsOfContact[i] += fdt;
            }

            for (int i = 0; i < boxDetectors.Length; i++)
            {

                BoxDetector detector = boxDetectors[i];
                
                if (!detector.active) { continue; }

                bool timer = detector.CheckTimer(fdt);

                Vector3 center = detector.center, size = detector.size*0.5f, direction = detector.direction;

                if (!detector.global)
                {
                    center = transform.position + transform.TransformDirection(detector.center);
                    size.Scale(transform.lossyScale);
                    direction = transform.TransformDirection(direction);
                }
                

                RaycastHit[] hits;
                Quaternion orientation = Quaternion.identity;
                if(detector.orientation != Vector3.zero) { orientation = Quaternion.LookRotation(detector.orientation); }
                hits = UnityEngine.Physics.BoxCastAll(center, size, direction, orientation, detector.detectionLength, detector.OnDetection.eventMask.layerMask);

                if(hits.Length == 0)
                {
                    if (detector.clear)
                    {
                        for(int j = detector.collisions.Length-1; j >=0; j--)
                        {
                            
                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Box, detector.collisions[j], detector.collisionDurations[j]);
                            if (detector.collisions[j].collider != null)
                            {
                                detector.OnStoppedDetecting?.Invoke(detector.collisions[j].collider.gameObject, data);
                            }
                            detector.collisionDurations[j] = 0;
                            detector.collisions[j] = new RaycastHit();
                        }

                        
                        detector.clear = false;
                        detector.OnNotDetecting?.Invoke(gameObject, new CollisionDetectorData());
                        if(detector.detectionLimit < 0)
                        {
                            detector.collisionDurations = new float[0];
                            detector.collisions = hits;
                        }
                    }
                    
                    continue;
                }

                if (detector.detectionLimit > 0)
                {
                    bool[] usedHit = new bool[hits.Length];

                    int l = detector.detectionLimit, cl = 0;
                    //1. Go find any collisions that have stuck around and handle them.
                    for (int j = 0; j < hits.Length; j++)
                    {
                        if(hits[j].collider == targetCollider) { continue; }

                        int id = detector.IsColliding(hits[j].collider);

                        if (id >= 0)
                        {
                            usedHit[j] = true;
                            cl++;
                            if (timer)
                            {
                                detector.collisions[id] = hits[j];
                                CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Box, hits[j], detector.collisionDurations[id]);
                                detector.WhileDetecting?.Invoke(hits[j].collider.gameObject, data);
                            }
                            if(cl >= l) { break; }
                        }
                    }

                    //2. Remove any old colliders we're not sensing anymore.
                    for (int j = detector.isStillColliding.Length - 1; j >= 0; j--)
                    {
                        
                        if (!detector.isStillColliding[j] && detector.collisions[j].collider != null)
                        {
                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Box, detector.collisions[j], detector.collisionDurations[j]);
                            detector.OnStoppedDetecting?.Invoke(detector.collisions[j].collider.gameObject, data);
                            detector.collisions[j] = new RaycastHit();
                            detector.collisionDurations[j] = 0f;
                        }
                    }

                    //3. If we didn't find enough old collisions to have handled our detecion limit, go through and add new hits to empty slots.
                    if (cl < l)
                    {
                        
                        for(int j = cl; j < l; j++)
                        {
                            int addID = -1;
                            for(int k = 0; k < hits.Length; k++)
                            {
                                if (hits[k].collider == targetCollider || usedHit[j]) { continue; }
                                addID = k; usedHit[k] = true; break; 
                            }

                            if(addID < 0) { break; }
                            
                            int addToID = -1;

                            for (int k = 0; k < l; k++)
                            {
                                if(detector.collisions[k].collider == null)
                                {
                                    addToID = k; break;
                                }
                            }

                            if(addToID < 0) { break; }

                            detector.collisions[addToID] = hits[addID];
                            detector.collisionDurations[addToID] = 0f;
                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Box, hits[addID], 0f);
                            detector.OnDetection?.Invoke(hits[addID].collider.gameObject, data);
                            detector.clear = true;
                        }
                    }
                }
                else
                {
                    //UNLIMITED POWER!!
                    List<RaycastHit> remainingCollisions = new List<RaycastHit>(detector.collisions);
                    List<float> remainingDurations = new List<float>(detector.collisionDurations);

                    for (int j = 0; j < hits.Length; j++)
                    {

                        if (hits[j].collider == targetCollider) { continue; }

                        int id = detector.IsColliding(hits[j].collider);

                        CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Box, hits[j], 0);
                        if (id >= 0)
                        {
                            if (timer)
                            {
                                data.duration = detector.collisionDurations[id];
                                detector.collisions[id] = hits[j];
                                detector.WhileDetecting?.Invoke(hits[j].collider.gameObject, data);
                            }
                        }
                        else
                        {
                            remainingCollisions.Add(hits[j]);
                            remainingDurations.Add(0);
                            detector.OnDetection?.Invoke(hits[j].collider.gameObject, data);
                            detector.clear = true;
                        }
                    }

                    for (int j = detector.isStillColliding.Length - 1; j >= 0; j--)
                    {
                        CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Box, remainingCollisions[j], detector.collisionDurations[j]);
                        if (!detector.isStillColliding[j] && remainingCollisions[j].collider !=null)
                        {
                            detector.OnStoppedDetecting?.Invoke(remainingCollisions[j].collider.gameObject, data);
                            remainingCollisions.RemoveAt(j);
                            remainingDurations.RemoveAt(j);
                        }
                    }

                    detector.collisions = remainingCollisions.ToArray();
                    detector.collisionDurations = remainingDurations.ToArray();
                }
                
                detector.isStillColliding = new bool[detector.collisions.Length];
            }

            for (int i = 0; i < capsuleDetectors.Length; i++)
            {
                if (!capsuleDetectors[i].active) { continue; }

                CapsuleDetector detector = capsuleDetectors[i];
                bool timer = detector.CheckTimer(fdt);
                Vector3 a = detector.a, b = detector.b, direction = detector.direction;
                float radius = detector.radius;

                if (!detector.global)
                {
                    a = transform.position + transform.TransformDirection(detector.a);
                    b = transform.position + transform.TransformDirection(detector.b);
                    direction = transform.TransformDirection(direction);
                }

                RaycastHit[] hits;

                hits =UnityEngine.Physics.CapsuleCastAll(a, b, radius, direction, detector.detectionLength, 0);

                if (hits.Length == 0)
                {
                    if (detector.clear)
                    {
                        for (int j = detector.collisions.Length - 1; j >= 0; j--)
                        {

                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Capsule, detector.collisions[j], detector.collisionDurations[j]);
                            if (detector.collisions[j].collider != null)
                            {
                                detector.OnStoppedDetecting?.Invoke(detector.collisions[j].collider.gameObject, data);
                            }
                            detector.collisionDurations[j] = 0;
                            detector.collisions[j] = new RaycastHit();
                        }


                        detector.clear = false;
                        detector.OnNotDetecting?.Invoke(gameObject, new CollisionDetectorData());
                        if (detector.detectionLimit < 0)
                        {
                            detector.collisionDurations = new float[0];
                            detector.collisions = hits;
                        }
                    }

                    continue;
                }

                if (detector.detectionLimit > 0)
                {
                    bool[] usedHit = new bool[hits.Length];

                    int l = detector.detectionLimit, cl = 0;
                    //1. Go find any collisions that have stuck around and handle them.
                    for (int j = 0; j < hits.Length; j++)
                    {
                        if (hits[j].collider == targetCollider) { continue; }

                        int id = detector.IsColliding(hits[j].collider);

                        if (id >= 0)
                        {
                            usedHit[j] = true;
                            cl++;
                            if (timer)
                            {
                                detector.collisions[id] = hits[j];
                                CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Capsule, hits[j], detector.collisionDurations[id]);
                                detector.WhileDetecting?.Invoke(hits[j].collider.gameObject, data);
                            }
                            if (cl >= l) { break; }
                        }
                    }

                    //2. Remove any old colliders we're not sensing anymore.
                    for (int j = detector.isStillColliding.Length - 1; j >= 0; j--)
                    {

                        if (!detector.isStillColliding[j] && detector.collisions[j].collider != null)
                        {
                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Capsule, detector.collisions[j], detector.collisionDurations[j]);
                            detector.OnStoppedDetecting?.Invoke(detector.collisions[j].collider.gameObject, data);
                            detector.collisions[j] = new RaycastHit();
                            detector.collisionDurations[j] = 0f;
                        }
                    }

                    //3. If we didn't find enough old collisions to have handled our detecion limit, go through and add new hits to empty slots.
                    if (cl < l)
                    {

                        for (int j = cl; j < l; j++)
                        {
                            int addID = -1;
                            for (int k = 0; k < hits.Length; k++)
                            {
                                if (hits[k].collider == targetCollider || usedHit[j]) { continue; }
                                addID = k; usedHit[k] = true; break;
                            }

                            if (addID < 0) { break; }

                            int addToID = -1;

                            for (int k = 0; k < l; k++)
                            {
                                if (detector.collisions[k].collider == null)
                                {
                                    addToID = k; break;
                                }
                            }

                            if (addToID < 0) { break; }

                            detector.collisions[addToID] = hits[addID];
                            detector.collisionDurations[addToID] = 0f;
                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Capsule, hits[addID], 0f);
                            detector.OnDetection?.Invoke(hits[addID].collider.gameObject, data);
                            detector.clear = true;
                        }
                    }
                }
                else
                {
                    //UNLIMITED POWER!!
                    List<RaycastHit> remainingCollisions = new List<RaycastHit>(detector.collisions);
                    List<float> remainingDurations = new List<float>(detector.collisionDurations);

                    for (int j = 0; j < hits.Length; j++)
                    {

                        if (hits[j].collider == targetCollider) { continue; }

                        int id = detector.IsColliding(hits[j].collider);

                        CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Capsule, hits[j], 0);
                        if (id >= 0)
                        {
                            if (timer)
                            {
                                data.duration = detector.collisionDurations[id];
                                detector.collisions[id] = hits[j];
                                detector.WhileDetecting?.Invoke(hits[j].collider.gameObject, data);
                            }
                        }
                        else
                        {
                            remainingCollisions.Add(hits[j]);
                            remainingDurations.Add(0);
                            detector.OnDetection?.Invoke(hits[j].collider.gameObject, data);
                            detector.clear = true;
                        }
                    }

                    for (int j = detector.isStillColliding.Length - 1; j >= 0; j--)
                    {
                        CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Capsule, remainingCollisions[j], detector.collisionDurations[j]);
                        if (!detector.isStillColliding[j] && remainingCollisions[j].collider != null)
                        {
                            detector.OnStoppedDetecting?.Invoke(remainingCollisions[j].collider.gameObject, data);
                            remainingCollisions.RemoveAt(j);
                            remainingDurations.RemoveAt(j);
                        }
                    }

                    detector.collisions = remainingCollisions.ToArray();
                    detector.collisionDurations = remainingDurations.ToArray();
                }

                detector.isStillColliding = new bool[detector.collisions.Length];
            }

            for (int i = 0; i < sphereDetectors.Length; i++)
            {
                if (!sphereDetectors[i].active) { continue; }
                SphereDetector detector = sphereDetectors[i];
                bool timer = detector.CheckTimer(fdt);

                Vector3 origin = detector.origin, direction = detector.direction;
                float radius = detector.radius;
                if (!detector.global)
                {
                    origin = transform.TransformPoint(origin);
                    direction = transform.TransformDirection(direction);
                }

                RaycastHit[] hits;
                hits = UnityEngine.Physics.SphereCastAll( origin, radius, direction, detector.detectionLength, 0);

                if (hits.Length == 0)
                {
                    if (detector.clear)
                    {
                        for (int j = detector.collisions.Length - 1; j >= 0; j--)
                        {

                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Sphere, detector.collisions[j], detector.collisionDurations[j]);
                            if (detector.collisions[j].collider != null)
                            {
                                detector.OnStoppedDetecting?.Invoke(detector.collisions[j].collider.gameObject, data);
                            }
                            detector.collisionDurations[j] = 0;
                            detector.collisions[j] = new RaycastHit();
                        }


                        detector.clear = false;
                        detector.OnNotDetecting?.Invoke(gameObject, new CollisionDetectorData());
                        if (detector.detectionLimit < 0)
                        {
                            detector.collisionDurations = new float[0];
                            detector.collisions = hits;
                        }
                    }

                    continue;
                }

                if (detector.detectionLimit > 0)
                {
                    bool[] usedHit = new bool[hits.Length];

                    int l = detector.detectionLimit, cl = 0;
                    //1. Go find any collisions that have stuck around and handle them.
                    for (int j = 0; j < hits.Length; j++)
                    {
                        if (hits[j].collider == targetCollider) { continue; }

                        int id = detector.IsColliding(hits[j].collider);

                        if (id >= 0)
                        {
                            usedHit[j] = true;
                            cl++;
                            if (timer)
                            {
                                detector.collisions[id] = hits[j];
                                CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Sphere, hits[j], detector.collisionDurations[id]);
                                detector.WhileDetecting?.Invoke(hits[j].collider.gameObject, data);
                            }
                            if (cl >= l) { break; }
                        }
                    }

                    //2. Remove any old colliders we're not sensing anymore.
                    for (int j = detector.isStillColliding.Length - 1; j >= 0; j--)
                    {

                        if (!detector.isStillColliding[j] && detector.collisions[j].collider != null)
                        {
                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Sphere, detector.collisions[j], detector.collisionDurations[j]);
                            detector.OnStoppedDetecting?.Invoke(detector.collisions[j].collider.gameObject, data);
                            detector.collisions[j] = new RaycastHit();
                            detector.collisionDurations[j] = 0f;
                        }
                    }

                    //3. If we didn't find enough old collisions to have handled our detecion limit, go through and add new hits to empty slots.
                    if (cl < l)
                    {

                        for (int j = cl; j < l; j++)
                        {
                            int addID = -1;
                            for (int k = 0; k < hits.Length; k++)
                            {
                                if (hits[k].collider == targetCollider || usedHit[j]) { continue; }
                                addID = k; usedHit[k] = true; break;
                            }

                            if (addID < 0) { break; }

                            int addToID = -1;

                            for (int k = 0; k < l; k++)
                            {
                                if (detector.collisions[k].collider == null)
                                {
                                    addToID = k; break;
                                }
                            }

                            if (addToID < 0) { break; }

                            detector.collisions[addToID] = hits[addID];
                            detector.collisionDurations[addToID] = 0f;
                            CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Sphere, hits[addID], 0f);
                            detector.OnDetection?.Invoke(hits[addID].collider.gameObject, data);
                            detector.clear = true;
                        }
                    }
                }
                else
                {
                    //UNLIMITED POWER!!
                    List<RaycastHit> remainingCollisions = new List<RaycastHit>(detector.collisions);
                    List<float> remainingDurations = new List<float>(detector.collisionDurations);

                    for (int j = 0; j < hits.Length; j++)
                    {

                        if (hits[j].collider == targetCollider) { continue; }

                        int id = detector.IsColliding(hits[j].collider);

                        CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Sphere, hits[j], 0);
                        if (id >= 0)
                        {
                            if (timer)
                            {
                                data.duration = detector.collisionDurations[id];
                                detector.collisions[id] = hits[j];
                                detector.WhileDetecting?.Invoke(hits[j].collider.gameObject, data);
                            }
                        }
                        else
                        {
                            remainingCollisions.Add(hits[j]);
                            remainingDurations.Add(0);
                            detector.OnDetection?.Invoke(hits[j].collider.gameObject, data);
                            detector.clear = true;
                        }
                    }

                    for (int j = detector.isStillColliding.Length - 1; j >= 0; j--)
                    {
                        CollisionDetectorData data = new CollisionDetectorData(this, i, DetectorType.Sphere, remainingCollisions[j], detector.collisionDurations[j]);
                        if (!detector.isStillColliding[j] && remainingCollisions[j].collider != null)
                        {
                            detector.OnStoppedDetecting?.Invoke(remainingCollisions[j].collider.gameObject, data);
                            remainingCollisions.RemoveAt(j);
                            remainingDurations.RemoveAt(j);
                        }
                    }

                    detector.collisions = remainingCollisions.ToArray();
                    detector.collisionDurations = remainingDurations.ToArray();
                }

                detector.isStillColliding = new bool[detector.collisions.Length];
            }

        }

        private void OnDrawGizmosSelected()
        {
            for (int i = 0; i < boxDetectors.Length; i++)
            {

                BoxDetector detector = boxDetectors[i];

                if (!detector.active) { continue; }

                Vector3 center = detector.center, size = detector.size * 0.5f, direction = detector.direction;

                if (!detector.global)
                {
                    center = transform.TransformPoint(detector.center);
                    //size.Scale(transform.lossyScale);
                    direction = transform.TransformDirection(direction);
                }

                Matrix4x4 gizmoMatrix = Gizmos.matrix;

                Quaternion orientation = Quaternion.Euler(detector.orientation);

                Matrix4x4 cubeMatrix = Matrix4x4.TRS(center, orientation, Vector3.one);
                Gizmos.matrix = cubeMatrix;

                Gizmos.DrawWireCube(Vector3.zero, size);

                Gizmos.DrawLine(Vector3.zero, direction * detector.detectionLength);

                Gizmos.matrix = gizmoMatrix;

                
            }

            for (int i = 0; i < capsuleDetectors.Length; i++)
            {
                if (!capsuleDetectors[i].active) { continue; }

                CapsuleDetector detector = capsuleDetectors[i];

                Vector3 a = detector.a, b = detector.b, direction = detector.direction;
                float radius = detector.radius;

                if (!detector.global)
                {
                    a = transform.position + transform.TransformDirection(detector.a);
                    b = transform.position + transform.TransformDirection(detector.b);
                    direction = transform.TransformDirection(direction);
                }

                Vector3 center = (a + b) * 0.5f, abDir = (b-a).normalized;
                Gizmos.DrawWireSphere(a, radius);
                Gizmos.DrawWireSphere(b, radius);
                Gizmos.DrawLine(a, b);
                Gizmos.DrawLine(center, center + direction * detector.detectionLength);
            }

            for (int i = 0; i < sphereDetectors.Length; i++)
            {
                if (!sphereDetectors[i].active) { continue; }
                SphereDetector detector = sphereDetectors[i];

                Vector3 origin = detector.origin, direction = detector.direction;
                float radius = detector.radius;
                if (!detector.global)
                {
                    origin = transform.TransformPoint(origin);
                    direction = transform.TransformDirection(direction);
                }

                Gizmos.DrawWireSphere(origin, radius);
                Gizmos.DrawLine(origin, origin + direction * detector.detectionLength);
            }
        }


    }

    [System.Serializable]
    public struct CollisionSensorData
    {
        public CollisionSensor sensor;
        public int colliderID;
        public Collision collision;
        public float duration;

        public CollisionSensorData(CollisionSensor s, int id, Collision c, float d)
        {
            sensor = s;
            colliderID = id;
            collision = c;
            duration = d;
        }
    }

    public delegate void CollisionSensorDataHandler(CollisionSensorData sensorData);

    [System.Serializable]
    public class CollisionSensorEvent : SensorEvent<CollisionSensorData>
    {
        public event CollisionSensorDataHandler sensorEvent;

        public override bool Invoke(GameObject targetObject, CollisionSensorData eventData)
        {
            if (base.Invoke(targetObject, eventData))
            {
                sensorEvent?.Invoke(eventData);
                return true;
            }
            return false;
        }
    }

    

    public enum DetectorType { Box, Capsule, Sphere}

    [System.Serializable]
    public abstract class CollisionDetector
    {
        public string name;

        public bool global, active, clear;

        public float detectionTimer = 0.1f, detectionLength;

        public Vector3 direction;

        public int detectionLimit = 0;
        
        public CollisionDetectorEvent OnDetection, WhileDetecting, OnStoppedDetecting, OnNotDetecting;

        protected float currentTimer = 0f;

        public RaycastHit[] collisions = new RaycastHit[0];
        public float[] collisionDurations = new float[0];
        public bool[] isStillColliding = new bool[0];

        public bool CheckTimer(float delta)
        {
            for(int i = collisions.Length-1; i >=0; i--)
            {
                if(collisions[i].collider == null) { continue; }
                collisionDurations[i] += delta;
            }

            if(detectionTimer <=0) { return true; }

            currentTimer += delta;
            if(currentTimer >= detectionTimer)
            {
                currentTimer -= detectionTimer;
                return true;
            }
            return false;
        }

        public int IsColliding(Collider obj)
        {
            for(int i = 0; i < collisions.Length; i++)
            {
                if (collisions[i].collider == obj) { isStillColliding[i] = true;  return i; }
            }
            return -1;
        }
        
    }

    [System.Serializable]
    public class BoxDetector: CollisionDetector
    {
        public Vector3 center, size, orientation;
    }

    [System.Serializable]
    public class CapsuleDetector: CollisionDetector
    {
        public Vector3 a, b;
        public float radius;
    }

    [System.Serializable]
    public class SphereDetector : CollisionDetector
    {
        public Vector3 origin;
        public float radius;
    }


    [System.Serializable]
    public struct CollisionDetectorData
    {
        public CollisionSensor sensor;
        public int detectorID;
        public DetectorType detectorType;
        public RaycastHit detection;
        public float duration;

        public CollisionDetectorData(CollisionSensor s, int id, DetectorType type, RaycastHit c, float d)
        {
            sensor = s;
            detectorID = id;
            detection = c;
            detectorType = type;
            duration = d;
        }
    }

    public delegate void CollisionDetectorDataHandler(CollisionDetectorData detectorData);

    [System.Serializable]
    public class CollisionDetectorEvent : SensorEvent<CollisionDetectorData>
    {
        public event CollisionDetectorDataHandler sensorEvent;

        public override bool Invoke(GameObject targetObject, CollisionDetectorData eventData)
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

