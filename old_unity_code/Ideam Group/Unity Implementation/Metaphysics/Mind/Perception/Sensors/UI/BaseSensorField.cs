using System;

using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    public abstract class BaseSensorField<T> : UserInterface<T>
        where T : BaseSensor
    {
        protected VisualElement baseSensorUI;
        protected Toggle isSensing;
        protected Foldout sensorEvents;
        protected ScrollView sensorEventScroll;

        public VisualTreeAsset baseSensorUXML;

        public override event Action<UserInterface> OnOpenUI, OnCloseUI;
        public override void SetSourceObject(T newObject)
        {
            sourceObject = newObject;

        }

        public override void SubscribeToObjectEvents()
        {

        }

        public override void UnsubscribeFromObjectEvents()
        {
        }

        public virtual VisualElement GenerateBaseSensorUI()
        {
            if (baseSensorUXML == null)
            {
                //baseSensorUI = Ideam.DefaultUI.GetUXML(this, "Base Sensor");
                if (baseSensorUXML == null) { return null; }
            }

            baseSensorUI = baseSensorUXML.CloneTree();
           
            isSensing = baseSensorUI.Q<Toggle>("IsSensing");
            isSensing.value = sourceObject.IsSensing;
            isSensing.RegisterValueChangedCallback(evt => { sourceObject.IsSensing = evt.newValue; });

            sensorEvents = baseSensorUI.Q<Foldout>("ShowSensorEvents");
            sensorEventScroll = baseSensorUI.Q<ScrollView>("SensorEventScroll");
            GenerateSensorEventsUI();

            return baseSensorUI;

        }
        
        public virtual void GenerateSensorEventsUI()
        {
            sensorEventScroll.Clear();
        }

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }

            root.Insert(0,GenerateBaseSensorUI());

            return true;
        }

    }

    public class EventMaskElement
    {
        public VisualElement element;
        public DropElement maskDrop;
        public DropList contextManagerDrop, contextMask;
        public LayersDrop layerMask;
        public TagsDrop tagsMask;

        public EventMask eventMask;
        public EventMaskElement(EventMask eventMask)
        {
            this.eventMask = eventMask;

            maskDrop = new DropElement();
            maskDrop.AddToClassList("indicatorLight");
            element = new VisualElement();
            maskDrop.SetDropContent(element);

            Label label = new Label("Event Mask");
            element.Add(label);

            layerMask = new LayersDrop();
            layerMask.AllowMultiSelect = true;
            element.Add(layerMask);

            layerMask.OnSelect += evt => { eventMask.layerMask = layerMask.LayerMask; };

            tagsMask = new TagsDrop();
            tagsMask.AllowMultiSelect = true;
            element.Add(tagsMask);

            tagsMask.OnSelect += evt => { eventMask.tagMask = tagsMask.Tags; };

        }
       
    }
}