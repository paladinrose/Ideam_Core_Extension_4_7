using System;
using UnityEngine;
using UnityEngine.UIElements;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Unity.Science.Formal.Mathematics
{
    public class Spline2Element : SplineBaseUI<Spline2>
    {
        public override event Action<int> OnSelectPoint, OnSelectSection;

        public event Action<XYToXYZ> OnAxisConversion;

        Vector2Field pointField, foreHandleField, backHandleField;
        Foldout useForeHandle, useBackHandle;
        EnumField axisConversionField;

        public Spline2Element(Spline2 spline) : base(spline)
        {
            axisConversionField = new EnumField("Axis Conversion", spline.axisConversion);
            axisConversionField.RegisterValueChangedCallback(SetAxisConversion);
            Add(axisConversionField);
        }
        
        private void SetAxisConversion(ChangeEvent<Enum> evt)
        {
            XYToXYZ newValue = (XYToXYZ)evt.newValue;
            if(newValue != spline.axisConversion)
            {
                spline.axisConversion = newValue;
                OnAxisConversion?.Invoke(newValue);
            }
        }

        public override void SelectPoint(int newPointID)
        {
            if(newPointID == selectedPoint) { return; }

            selectedPoint = newPointID;
            pointElement.Clear();

            if (selectedPoint == -1)
            {
                pointElement.style.display = DisplayStyle.None;

                OnSelectPoint?.Invoke(selectedPoint);

                return;
            }

            pointElement.style.display = DisplayStyle.Flex;
            Point2 point = spline.points[selectedPoint];

            pointField = new Vector2Field("Point " + selectedPoint);
            pointField.value = point.point.ToVector2();
            pointField.RegisterValueChangedCallback(SetPoint);
            pointElement.Add(pointField);


            useForeHandle = new Foldout() { text = "Use ForeHandle", value = point.useForeHandle };
            useForeHandle.RegisterValueChangedCallback(SetUseForeHandle);
            pointElement.Add(useForeHandle);

            foreHandleField = new Vector2Field("ForeHandle");
            foreHandleField.value = point.foreHandle.ToVector2();
            foreHandleField.RegisterValueChangedCallback(SetForeHandle);
            useForeHandle.Add(foreHandleField);


            useBackHandle = new Foldout() { text = "Use BackHandle", value = point.useBackHandle };
            useBackHandle.RegisterValueChangedCallback(SetUseBackHandle);
            pointElement.Add(useBackHandle);

            backHandleField = new Vector2Field("BackHandle");
            backHandleField.value = point.backHandle.ToVector2();
            backHandleField.RegisterValueChangedCallback(SetBackHandle);
            useBackHandle.Add(backHandleField);

            OnSelectPoint?.Invoke(selectedPoint);
        }

        public void UpdatePointFields()
        {
            Point2 point = spline.points[selectedPoint];
            pointField.SetValueWithoutNotify(point.point.ToVector2());
            
            foreHandleField.SetValueWithoutNotify(point.foreHandle.ToVector2());
            backHandleField.SetValueWithoutNotify(point.backHandle.ToVector2());
            useForeHandle.SetValueWithoutNotify(point.useForeHandle);
            useBackHandle.SetValueWithoutNotify(point.useBackHandle);
        }

        private void SetPoint(ChangeEvent<Vector2> newPointValue)
        {
            Point2 point = spline.points[selectedPoint];
            point.point = newPointValue.newValue.ToFloat2();
            spline.points[selectedPoint] = point;
            if(spline.wrap && selectedPoint == 0)
            {
                int l = spline.points.Length - 1;
                point = spline.points[l];
                point.point = newPointValue.newValue.ToFloat2();
                spline.points[l] = point;
            }
            
            spline.anchorsDirty = true;
            spline.splineDirty = true;
            
        }

        private void SetUseForeHandle(ChangeEvent<bool> useForeHandleValue)
        {
            Point2 point = spline.points[selectedPoint];
            point.useForeHandle = useForeHandleValue.newValue;
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }
        private void SetUseBackHandle(ChangeEvent<bool> useBackHandleValue)
        {
            Point2 point = spline.points[selectedPoint];
            point.useBackHandle = useBackHandleValue.newValue;
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }
        private void SetForeHandle(ChangeEvent<Vector2> newForeHandleValue)
        {
            Point2 point = spline.points[selectedPoint];
            point.foreHandle = newForeHandleValue.newValue.ToFloat2();
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }

        private void SetBackHandle(ChangeEvent<Vector2> newBackHandleValue)
        {
            Point2 point = spline.points[selectedPoint];
            point.backHandle = newBackHandleValue.newValue.ToFloat2();
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }

    }
}