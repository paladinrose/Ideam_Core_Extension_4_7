using System;

using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    [Serializable]
    public class UnitySight : UnitySense<UnityVisualSignal>
    {

        public static int visualSeme, motionSeme;

        public TriggerSensor peripheralVision;
        public RaySensor linesOfSight;


        public override void Setup(GameObject sourceObject)
        {
            if (peripheralVision)
            {
                peripheralVision.OnTriggerStart.hitEvent += StartSeeingPeripheral;
                peripheralVision.OnTriggerStop.hitEvent += StopSeeingPeripheral;
            }

            if (linesOfSight)
            {
                for(int i =0; i < linesOfSight.rays.Length; i++)
                {
                    linesOfSight.rays[i].OnStartHitting.sensorEvent += StartSeeing;
                    linesOfSight.rays[i].OnStopHitting.sensorEvent += StopSeeing;
                }
            }
        }

        public override void EnableSensing()
        {
            if (peripheralVision) peripheralVision.IsSensing = true;
            if (linesOfSight) linesOfSight.IsSensing = true;
        }

        public override void DisableSensing()
        {
            if (peripheralVision) peripheralVision.IsSensing = false;
            if (linesOfSight) linesOfSight.IsSensing = false;
        }


        public virtual void StartSeeing(RaySensorData sensorData)
        {
            // 1. Create a UnityVisualSignal.
            // 2. Connect it to a visual environment (for noise, etc.)
            // 3. 
            UnityGamePiece sensedPiece = sensorData.hit.collider.gameObject.GetComponent<UnityGamePiece>();
            if (sensedPiece) BeginSensing(sensedPiece);
        }

        public virtual void StopSeeing(RaySensorData sensorData)
        {

        }

        public virtual void StartSeeingPeripheral(GameObject obj)
        {
            UnityGamePiece sensedPiece = obj.GetComponent<UnityGamePiece>();
            if (sensedPiece) BeginSensing(sensedPiece);
        }

        public virtual void StopSeeingPeripheral(GameObject obj)
        {
            UnityGamePiece sensedPiece = obj.GetComponent<UnityGamePiece>();
            if (sensedPiece) StopSensing(sensedPiece);
        }
    }
}