using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.UI;
using UnityEngine.UIElements;
using System;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    [UserInterface(typeof(SensorUnityEvents))]
    public class SensorUnityEventsUI : UserInterface<SensorUnityEvents>
    {
        public override void SubscribeToObjectEvents()
        {
        }

        public override void UnsubscribeFromObjectEvents()
        {
        }

        ObjectSelectorField sensorSelector;
        public Action OnSensorSet;

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }

            sensorSelector = root.Q<ObjectSelectorField>("SensorSelector");
            sensorSelector.Initialize<BaseSensor>(SelectSensor, sourceObject.Sensor, sourceObject.gameObject);

            return true;
        }

        private void SelectSensor(BaseSensor selection)
        {
            if(sourceObject.Sensor == selection) { return; }
            sourceObject.Sensor = selection;
            OnSensorSet?.Invoke();
        }
    }
}