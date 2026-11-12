using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    public class DetectableVisualField : UserInterface<DetectableVisual>
    {
        public override void SubscribeToObjectEvents()
        {
        }

        public override void UnsubscribeFromObjectEvents()
        {
        }

        public void GenerateElementFields()
        {
            EventMaskElement onSeenElement = new EventMaskElement(sourceObject.OnSeen.eventMask);
            onSeenElement.maskDrop.Label = "On Seen";
            sourceObject.OnSeen.sensorEvent += OnSeenEvent;
            ui.Add(onSeenElement.maskDrop);

            EventMaskElement whileSeenElement = new EventMaskElement(sourceObject.WhileSeen.eventMask);
            whileSeenElement.maskDrop.Label = "While Seen";
            sourceObject.WhileSeen.sensorEvent += WhileSeenEvent;
            ui.Add(whileSeenElement.maskDrop);

            EventMaskElement onStoppedBeingSeenElement = new EventMaskElement(sourceObject.OnStoppedBeingSeen.eventMask);
            onStoppedBeingSeenElement.maskDrop.Label = "On Stopped Being Seen";
            sourceObject.OnStoppedBeingSeen.sensorEvent += OnStoppedBeingSeenEvent;
            ui.Add(onStoppedBeingSeenElement.maskDrop);

            EventMaskElement onNotSeenElement = new EventMaskElement(sourceObject.OnNotSeen.eventMask);
            onNotSeenElement.maskDrop.Label = "On Not Seen";
            sourceObject.OnNotSeen.sensorEvent += OnNotSeenEvent;
            ui.Add(onNotSeenElement.maskDrop);
        }

        private void OnSeenEvent(RaySensorData sensorData)
        {
        }

        private void WhileSeenEvent(RaySensorData sensorData)
        {
        }

        private void OnStoppedBeingSeenEvent(RaySensorData sensorData)
        {
        }

        private void OnNotSeenEvent(RaySensorData sensorData)
        {
        }
    }
}