using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class ColliderControl : BaseControl
    {
        public event Action<int, GameObject> OnStartColliding, OnStopColliding;
        public event Action<int> OnCollisionUpdate;


        [System.Serializable]
        public struct ColliderData
        {
            public bool isColliding;
            public float collisionStrength;
           
            public Transform collider, colliderTarget, colliderTargetParent;
            public Rigidbody colliderBody, colliderTargetBody;

            public CollisionSensor collisionSensor;


            public Vector3 collisionPoint, collisionNormal;

            public ColliderData(ColliderData original)
            {
                isColliding = original.isColliding;
                collisionStrength = original.collisionStrength;
                
                collider = original.collider;
                colliderTarget = original.colliderTarget;
                colliderTargetParent = original.colliderTargetParent;
                colliderBody = original.colliderBody;
                colliderTargetBody = original.colliderTargetBody;
                collisionSensor = original.collisionSensor;
               
                collisionNormal = original.collisionNormal;
                collisionPoint = original.collisionPoint;
            }
        }


        public ColliderData[] collisionData = new ColliderData[0];

        public int StartColliding(ColliderData collision)
        {
            //if (Collider.ColliderTarget == null) { return; }

            //Debug.Log("Trying to Collider to " + Collider.ColliderTarget.name);

            int collisionID = -1, firstEmpty = -1;
            for (int i = 0; i < collisionData.Length; i++)
            {
                if (collisionData[i].isColliding)
                {
                    if (collisionData[i].collider == collision.collider && collisionData[i].colliderTarget == collision.colliderTarget)
                    {
                        collisionID = i;
                        break;
                    }

                }
                else if (firstEmpty < 0 && collisionData[i].collider == collision.collider) { firstEmpty = i; }
            }

            if (collisionID >= 0)
            {
                //We're already Collidering to this object with this Colliderer. 
                //We want to update the Collidering values then finish.
                ColliderData updatedCollider = collisionData[collisionID];

                collisionData[collisionID] = updatedCollider;
                return collisionID;
            }
            else if (firstEmpty >= 0)
            {
                ColliderData data = new ColliderData(collision);

                if (data.colliderTarget)
                {
                    data.colliderTargetBody = data.colliderTarget.GetComponent<Rigidbody>();

                    if (data.colliderBody)
                    {
                        
                        if (data.colliderTargetBody)
                        {
                           
                        }
                        else
                        {
                            
                        }
                    }
                    else
                    {
                       
                    }
                }

                collisionData[firstEmpty] = data;
                GameObject ColliderObj = null;
                if (data.colliderTarget) { ColliderObj = data.colliderTarget.gameObject; }
                OnStartColliding?.Invoke(firstEmpty, ColliderObj);
                return firstEmpty;
            }

            return -1;
        }

        public void UpdateColliderValues(int id, float collisionStrength)
        {
            ColliderData collision = collisionData[id];
            collision.collisionStrength = collisionStrength;
            collisionData[id] = collision;
        }

        public void StopColliding(int id)
        {
            if (id >= collisionData.Length || id < 0) return;

            ColliderData Collider = collisionData[id];
            if (Collider.colliderTarget)
            {
                GameObject wasCollidering = Collider.colliderTarget.gameObject;
                Collider.collisionStrength = 0;
                Collider.isColliding = false;
                
                Collider.colliderTarget = null;
                Collider.colliderTargetBody = null;
                Collider.colliderTargetParent = null;
                collisionData[id] = Collider;
                OnStopColliding?.Invoke(id, wasCollidering);
            }
            collisionData[id] = Collider;
        }

        public void ColliderUpdate()
        {
            for (int i = 0; i < collisionData.Length; i++)
            {
                ColliderData Collider = collisionData[i];

                if (!Collider.collider || !Collider.colliderTarget) { return; }
                bool didUpdate = false;
               
                if (didUpdate) { OnCollisionUpdate?.Invoke(i); }
            }
        }

        private void DirectSetCollider(ColliderData collision)
        {
            if (controlFlags.HasFlag(ControlFlags.Physical))
            {
            }
            else
            {
            }
        }

    }
}