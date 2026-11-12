using System;

using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    [Serializable]
    public class UnityHearing : UnitySense<UnityAudioSignal>
    {
        public AudioSensor hearing;
        public override void Setup(GameObject sourceObject)
        {
            
        }

        public override void EnableSensing()
        {
            if (hearing) hearing.IsSensing = true;
        }

        public override void DisableSensing()
        {
            if (hearing) hearing.IsSensing = false;
        }
    }
}