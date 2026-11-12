using System;
using System.Collections.Generic;

using Ideam.Unity.UI;


using UnityEngine;
using UnityEngine.UIElements;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    public class RaySensorField : BaseSensorField<RaySensor>
    {
        public VisualTreeAsset rayUXML;
        protected Foldout showRays;
        protected ScrollView rayScroll;
        protected Button newRay;

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }

            showRays = root.Q<Foldout>("ShowRays");
            rayScroll = showRays.Q<ScrollView>("RayScroll");

            newRay = showRays.Q<Button>("NewRay");
            newRay.clicked += () =>
            {

                SensorRay[] rays = sourceObject.rays;
                int l = rays.Length;
                System.Array.Resize<SensorRay>(ref rays, l + 1);
                rays[l] = new SensorRay() { name = "Ray " + (l + 1).ToString() };
                sourceObject.rays = rays;
                rayScroll.Add(GenerateRayField(l));
            };

            GenerateRayFields();

            return true;
        }

        public override void GenerateSensorEventsUI()
        {
            sensorEventScroll.Clear();

            
        }

        

        protected virtual void GenerateRayFields()
        {
            rayScroll.Clear();
            for(int i = 0; i < sourceObject.rays.Length; i++)
            {
                rayScroll.Add(GenerateRayField(i));
            }
        }

        protected virtual VisualElement GenerateRayField(int id)
        {
            SensorRay ray = sourceObject.rays[id];

            VisualElement rayUI = rayUXML.CloneTree();
            Foldout showRay = rayUI.Q<Foldout>("ShowRay");
            showRay.text = ray.name;

            TextField nameField = rayUI.Q<TextField>("NameField");
            nameField.value = ray.name;
            nameField.RegisterValueChangedCallback(evt => { ray.name = evt.newValue; showRay.text = ray.name; });

            Button removeButton = rayUI.Q<Button>("RemoveRay");
            removeButton.clicked += () => 
            {
                List<SensorRay> newRays = new List<SensorRay>(sourceObject.rays);
                newRays.Remove(ray);
                sourceObject.rays = newRays.ToArray();
                rayScroll.Remove(rayUI);
            };

            LayersDrop layerMask = rayUI.Q<LayersDrop>("LayerMask");
            layerMask.LayerMask = ray.layerMask;
            layerMask.OnSelect += evt => { ray.layerMask = layerMask.LayerMask; };

            Toggle onStartToggle = rayUI.Q<Toggle>("CastOnStart");
            onStartToggle.value = ray.castOnStart;
            onStartToggle.RegisterValueChangedCallback(evt => { ray.castOnStart = evt.newValue; });

            Toggle onActiveToggle = rayUI.Q<Toggle>("CastOnActive");
            onActiveToggle.value = ray.castOnActive;
            onActiveToggle.RegisterValueChangedCallback(evt => { ray.castOnActive = evt.newValue; });

            ObjectSelectorField originSelector = rayUI.Q<ObjectSelectorField>("OriginSelector");
            originSelector.Initialize<Transform>(evt => { ray.originTransform = evt; },ray.originTransform, sourceObject.gameObject);

            FloatField maxLength = rayUI.Q<FloatField>("LengthField");
            maxLength.value = ray.length;
            maxLength.RegisterValueChangedCallback(evt => { ray.length = maxLength.value; });

            Toggle scaleLengthToggle = rayUI.Q<Toggle>("ScaleLengthField");
            scaleLengthToggle.value = ray.scaleLengthToOriginTransform;
            scaleLengthToggle.RegisterValueChangedCallback(evt => { ray.scaleLengthToOriginTransform = evt.newValue; });

            Vector3Field originField = rayUI.Q<Vector3Field>("OriginField");
            originField.value = ray.origin;
            originField.RegisterValueChangedCallback(evt => { ray.origin = evt.newValue; });

            EnumField originSpaceDrop = rayUI.Q<EnumField>("OriginSpaceDrop");
            originSpaceDrop.Init(ray.originSpace);
            originSpaceDrop.RegisterValueChangedCallback( evt => { ray.originSpace = (Space)evt.newValue; });

            Vector3Field directionField = rayUI.Q<Vector3Field>("DirectionField");
            directionField.value = ray.direction;
            directionField.RegisterValueChangedCallback(evt => { ray.direction = evt.newValue; });

            EnumField directionSpaceDrop = rayUI.Q<EnumField>("DirectionSpaceDrop");
            directionSpaceDrop.Init(ray.directionSpace);
            directionSpaceDrop.RegisterValueChangedCallback( evt => { ray.directionSpace = (Space)evt.newValue; });

            return rayUI;
        }

        #region Sensor Event Reactions
       
        #endregion
    }
}