using UnityEngine;
using UnityEngine.Events;
using System.Collections;
using System.Collections.Generic;
using System;

using UnityEngine.InputSystem;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    [System.Serializable, AddComponentMenu("Ideam/Sensors/Input Sensor")]
    public class InputSensor : BaseSensor
    {
        
        void Start()
        {
            
        }

        private void FixedUpdate()
        {

            if (!IsSensing) { return; }
          
        }
    }

    [System.Serializable]
    public class InputSystemSensorEvent : SensorEvent<InputSensor>
    {
        public override bool Invoke(GameObject targetObject, InputSensor eventData)
        {
            return false;
        }

        public bool Invoke(GameObject targetObject, InputSensor eventData, float axisValue)
        {
            return false;
        }
    }

}
