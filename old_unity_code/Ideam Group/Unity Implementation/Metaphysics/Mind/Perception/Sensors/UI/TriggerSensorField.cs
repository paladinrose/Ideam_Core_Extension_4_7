using Ideam.Unity.UI;

using UnityEngine.UIElements;
using System;
using System.Collections.Generic;
using UnityEngine;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    public class TriggerSensorField : BaseSensorField<TriggerSensor>
    {
        //EventMaskElement onFirstTriggerMask, onStartTriggerMask, whileTriggeredMask, onStopTriggerMask, onLastTriggerStopMask;
        DropList collidersInTriggerList;
       

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }

            List<string> colliderNames = new List<string>();
            List<int> colliderIDs = new List<int>();

            collidersInTriggerList = root.Q<DropList>();
            collidersInTriggerList.BuildList(colliderNames.ToArray(), colliderIDs.ToArray());

            return true;
        }
        public override void GenerateSensorEventsUI()
        {
            sensorEventScroll.Clear();


            EventMaskElement onFirstTriggerMask = new EventMaskElement(sourceObject.OnFirstTriggerStart.eventMask);
            onFirstTriggerMask.maskDrop.Label = "On First Trigger";
            sourceObject.OnFirstTriggerStart.sensorEvent += FirstTriggerSensorEvent;
            sourceObject.OnFirstTriggerStart.hitEvent += FirstTriggerHitEvent;
            sensorEventScroll.Add(onFirstTriggerMask.maskDrop);

            EventMaskElement onStartTriggerMask = new EventMaskElement(sourceObject.OnTriggerStart.eventMask);
            onStartTriggerMask.maskDrop.Label = "On Trigger Start";
            sourceObject.OnTriggerStart.sensorEvent += TriggeredSensorEvent;
            sourceObject.OnTriggerStart.hitEvent += TriggeredHitEvent;
            sensorEventScroll.Add(onStartTriggerMask.maskDrop);

            EventMaskElement whileTriggeredMask = new EventMaskElement(sourceObject.WhileTriggered.eventMask);
            whileTriggeredMask.maskDrop.Label = "While Triggered";
            sourceObject.WhileTriggered.sensorEvent += WhileTriggeredSensorEvent;
            sourceObject.WhileTriggered.hitEvent += WhileTriggeredHitEvent;
            sensorEventScroll.Add(whileTriggeredMask.maskDrop);

            EventMaskElement onStopTriggerMask = new EventMaskElement(sourceObject.OnTriggerStop.eventMask);
            onStopTriggerMask.maskDrop.Label = "On Stop Trigger";
            sourceObject.OnTriggerStop.sensorEvent += StopTriggerSensorEvent;
            sourceObject.OnTriggerStop.hitEvent += StopTriggerHitEvent;
            sensorEventScroll.Add(onStopTriggerMask.maskDrop);

            EventMaskElement onLastTriggerStopMask = new EventMaskElement(sourceObject.OnLastTriggerStop.eventMask);
            onLastTriggerStopMask.maskDrop.Label = "On Last Trigger Stop";
            sourceObject.OnLastTriggerStop.sensorEvent += LastTriggerStopSensorEvent;
            sourceObject.OnLastTriggerStop.hitEvent += LastTriggerStopHitEvent;
            sensorEventScroll.Add(onLastTriggerStopMask.maskDrop);
        }

        private void FirstTriggerHitEvent(GameObject obj)
        {

        }

        private void FirstTriggerSensorEvent(TriggerSensorData sensorData)
        {

        }


        private void TriggeredHitEvent(GameObject obj)
        {
           
        }

        private void TriggeredSensorEvent(TriggerSensorData sensorData)
        {
            
        }


        private void WhileTriggeredSensorEvent(TriggerSensorData sensorData)
        {
            
        }

        private void WhileTriggeredHitEvent(GameObject obj)
        {
           
        }


        private void StopTriggerSensorEvent(TriggerSensorData sensorData)
        {
            
        }

        private void StopTriggerHitEvent(GameObject obj)
        {
            
        }


        private void LastTriggerStopSensorEvent(TriggerSensorData sensorData)
        {
            
        }

        private void LastTriggerStopHitEvent(GameObject obj)
        {
           
        }
    }
}