using Ideam.Unity.UI;

using UnityEngine.UIElements;
using System;
using System.Collections.Generic;
using UnityEngine;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    public class JointSensorField : BaseSensorField<JointSensor>
    {

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }


            return true;
        }


        public override void GenerateSensorEventsUI()
        {
            sensorEventScroll.Clear();


            EventMaskElement onJointBreakMask = new EventMaskElement(sourceObject.JointBreak.eventMask);
            onJointBreakMask.maskDrop.Label = "On Joint Break";
            sourceObject.JointBreak.sensorEvent += JointBreakSensorEvent;
            sensorEventScroll.Add(onJointBreakMask.maskDrop);

        }


        private void JointBreakSensorEvent(JointSensorData sensorData)
        {

        }


    }
}