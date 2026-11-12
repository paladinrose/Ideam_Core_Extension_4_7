using System;
using UnityEngine;
using UnityEngine.UIElements;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Unity.Science.Formal.Mathematics
{
    public class Spline3Element : SplineBaseUI<Spline3>
    {
        public override event Action<int> OnSelectPoint, OnSelectSection;

        Vector3Field pointField, foreHandleField, backHandleField;
        Foldout useForeHandle, useBackHandle;

        public Spline3Element(Spline3 spline) : base(spline)
        {
                
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
            Point3 point = spline.points[selectedPoint];
            
            pointField = new Vector3Field("Point " + selectedPoint);
            pointField.value = point.point.ToVector3();
            pointField.RegisterValueChangedCallback(SetPoint);
            pointElement.Add(pointField);
            

            useForeHandle = new Foldout() { text = "Use ForeHandle", value = point.useForeHandle };
            useForeHandle.RegisterValueChangedCallback(SetUseForeHandle);
            pointElement.Add(useForeHandle);

            foreHandleField = new Vector3Field("ForeHandle");
            foreHandleField.value = point.foreHandle.ToVector3();
            foreHandleField.RegisterValueChangedCallback(SetForeHandle);
            useForeHandle.Add(foreHandleField);


            useBackHandle = new Foldout() { text = "Use BackHandle", value = point.useBackHandle };
            useBackHandle.RegisterValueChangedCallback(SetUseBackHandle);
            pointElement.Add(useBackHandle);

            backHandleField = new Vector3Field("BackHandle");
            backHandleField.value = point.backHandle.ToVector3();
            backHandleField.RegisterValueChangedCallback(SetBackHandle);
            useBackHandle.Add(backHandleField);

            OnSelectPoint?.Invoke(selectedPoint);
        }

        public void UpdatePointFields()
        {
            Point3 point = spline.points[selectedPoint];
            pointField.SetValueWithoutNotify(point.point.ToVector3());
            foreHandleField.SetValueWithoutNotify(point.foreHandle.ToVector3());
            backHandleField.SetValueWithoutNotify(point.backHandle.ToVector3());
            useForeHandle.SetValueWithoutNotify(point.useForeHandle);
            useBackHandle.SetValueWithoutNotify(point.useBackHandle);
        }

        private void SetPoint(ChangeEvent<Vector3> newPointValue)
        {
            Point3 point = spline.points[selectedPoint];
            point.point = newPointValue.newValue.ToFloat3();
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }

        private void SetUseForeHandle(ChangeEvent<bool> useForeHandleValue) 
        {
            Point3 point = spline.points[selectedPoint];
            point.useForeHandle = useForeHandleValue.newValue;
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }
        private void SetUseBackHandle(ChangeEvent<bool> useBackHandleValue)
        {
            Point3 point = spline.points[selectedPoint];
            point.useBackHandle = useBackHandleValue.newValue;
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }
        private void SetForeHandle(ChangeEvent<Vector3> newForeHandleValue)
        {
            Point3 point = spline.points[selectedPoint];
            point.foreHandle = newForeHandleValue.newValue.ToFloat3();
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }

        private void SetBackHandle(ChangeEvent<Vector3> newBackHandleValue)
        {
            Point3 point = spline.points[selectedPoint];
            point.backHandle = newBackHandleValue.newValue.ToFloat3();
            spline.points[selectedPoint] = point;
            spline.splineDirty = true;
        }

    }
}