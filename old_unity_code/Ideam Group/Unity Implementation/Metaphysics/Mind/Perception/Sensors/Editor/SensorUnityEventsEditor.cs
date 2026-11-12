using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;
using UnityEngine.Events;

using UnityEditor;
using UnityEditor.UIElements;
using UnityEditor.Events;

using Ideam.Editor;


namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{ 
    [CustomEditor(typeof(SensorUnityEvents))]
    public class SensorUnityEventsEditor : EditorUserInterface<SensorUnityEvents, SensorUnityEventsUI>
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[]
            {
                "sensor", "unityEvents", "sensorType"
            }).ToArray();
        }

        VisualElement sensorEventsElement;
        Foldout[] rayFoldouts;
        PropertyField[] sensorEventFields;

        public override VisualElement CreateInspectorGUI()
        {
            base.CreateInspectorGUI();
            ui.OnSensorSet += CreateEventFields;

            sensorEventsElement = new VisualElement();
            root.Add(sensorEventsElement);

            CreateEventFields();
            return root;

        }

        private void CreateEventFields()
        {
            sensorEventsElement.Clear();
            if (editorObject.sensorType == "RaySensor")
            {
                RaySensor rs = (RaySensor)editorObject.Sensor;
                rayFoldouts = new Foldout[rs.rays.Length];
                for(int i =0; i < rs.rays.Length; i++)
                {
                    rayFoldouts[i] = new Foldout() { text = rs.rays[i].name, value = false };
                    sensorEventsElement.Add(rayFoldouts[i]);
                }
            }

            SerializedProperty uesp = serializedObject.FindProperty("unityEvents");

            int eventCount = 0, rayCount = 0;
            sensorEventFields = new PropertyField[uesp.arraySize];
            for(int i =0; i < uesp.arraySize; i++)
            {
                if (eventCount == 7)
                {
                    rayCount++;
                    eventCount = 0;
                }

                VisualElement eventElement = new VisualElement();
                eventElement.style.marginBottom = 10;

                string label = SetLabel(eventCount);
                Label lbl = new Label(label);
                eventElement.Add(lbl);
               
                PropertyField unityEventField = new PropertyField(uesp.GetArrayElementAtIndex(i));
                eventElement.Add(unityEventField);
                sensorEventFields[i] = unityEventField;
                

                if (editorObject.sensorType != "RaySensor")
                {
                    sensorEventsElement.Add(eventElement);
                }
                else
                {
                    rayFoldouts[rayCount].Add(eventElement);
                }
                eventCount++;
            }

        }


        string[] audioEvents = new string[] { "On Start Hearing", "On Hear", "On Stop Hearing" },
                 c2dEvents = new string[] { "On First Collision2D Start", "On Collision2D Start", "While 2DColliding", "On Collision2D Stop", "On Last Collision2D Stop" }, 
                 collisionEvents = new string[] { "On First Collision Start", "On Collision Start", "While Colliding", "On Collision Stop", "On Last Collision Stop" },
                 jointEvents = new string[] { "On Joint Break" },
                 rayEvents = new string[] { "On Start Casting", "While Casting", "On Stop Casting", "On Start Hitting", "While Hitting", "On Stop Hitting", "On Not Hitting" },
                 t2DEvents = new string[] { "On First Trigger2D Start", "On Trigger2D Start", "While 2DTriggered", "On Trigger2D Stop", "On Last Trigger2D Stop" }, 
                 triggerEvents = new string[] { "On First Trigger Start", "On Trigger Start", "While Triggered", "On Trigger Stop", "On Last Trigger Stop" };

        protected string SetLabel(int eventID)
        {
            string sensorType = editorObject.sensorType;

            if (sensorType == "AudioSensor")
            {
                return audioEvents[eventID];
            }
            else if (sensorType == "Collision2DSensor")
            {
                return c2dEvents[eventID];
            }
            else if (sensorType == "CollisionSensor")
            {
                return collisionEvents[eventID];
            }
            else if (sensorType == "JointSensor")
            {
                return jointEvents[0];
            }
            else if (sensorType == "RaySensor")
            {
                return rayEvents[eventID];
            }
            else if (sensorType == "Trigger2DSensor")
            {
                return t2DEvents[eventID];
            }
            else if (sensorType == "TriggerSensor")
            {
                return triggerEvents[eventID];
            }
            else
            {
                return "Unity Event #" + eventID;
            }
        }

        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            ui.UXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Metaphysics/Mind/Perception/Sensors/UI/SensorUnityEvents.uxml");
        }
    }
}