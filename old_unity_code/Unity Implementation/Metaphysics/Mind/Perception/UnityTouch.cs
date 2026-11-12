using System;

using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    [Serializable]
    public class UnityTouch : UnitySense<UnityHapticSignal>
    {
        public TriggerSensor[] softTouchPoints;
        public CollisionSensor[] hardTouchPoints;

        public override void Setup(GameObject sourceObject)
        {

        }
        public override void EnableSensing()
        {

        }
        public override void DisableSensing()
        {

        }
    }
}