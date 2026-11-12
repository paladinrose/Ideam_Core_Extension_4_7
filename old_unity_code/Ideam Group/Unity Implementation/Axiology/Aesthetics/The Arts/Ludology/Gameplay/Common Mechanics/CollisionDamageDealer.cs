using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using System;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay
{
    public class CollisionDamageDealer : MonoBehaviour
    {
        [System.Serializable]
        public struct CollisionDamageData
        {
            public CollisionSensor sensor;
            public float damageMultiplier; 
            public Vector3 fullDamageNormal;
            public event Action<GameObject, float> OnDamage;

            public void OnHit(CollisionSensorData sensorData)
            {
                float damage = sensorData.collision.relativeVelocity.magnitude * damageMultiplier;
                float offAngle = Quaternion.Angle
                (
                    Quaternion.Euler(fullDamageNormal),
                    Quaternion.Euler(sensorData.collision.GetContact(0).normal)
                );

                offAngle /= 360f;
                damage *= (1f - Mathf.Abs(offAngle));
                OnDamage?.Invoke(sensorData.collision.gameObject, damage);
            }
        }

        public CollisionDamageData[] damageSources = new CollisionDamageData[0];
        private void Awake()
        {
            SetupOnHitEvents();
        }
        protected virtual void SetupOnHitEvents()
        {
            Debug.Log("Setting up On Hit Events");
            for(int i =0; i< damageSources.Length; i++)
            {
                int id = i;
                damageSources[i].sensor.OnCollisionStart.sensorEvent += damageSources[i].OnHit;
            }   
        }

    }
}