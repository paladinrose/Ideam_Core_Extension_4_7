using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Formal.Mathematics
{


    public class Spline3DrawingElement : SplineDrawingElement<Spline3>
    {
        #region UXML Factory

        public new class UxmlFactory : UxmlFactory<Spline3DrawingElement, UxmlTraits> { }

        public new class UxmlTraits : VisualElement.UxmlTraits
        {
            UxmlFloatAttributeDescription lWDescription = new UxmlFloatAttributeDescription { name = "Dropped", defaultValue = 2 };

            public override IEnumerable<UxmlChildElementDescription> uxmlChildElementsDescription
            {
                get { yield break; }
            }

            public override void Init(VisualElement ve, IUxmlAttributes bag, CreationContext cc)
            {
                base.Init(ve, bag, cc);
                var splineElement = ve as Spline3DrawingElement;

                splineElement.LineWidth = lWDescription.GetValueFromBag(bag, cc);
            }
        }

        #endregion

        protected override void GetLocalSmoothedPoints()
        {
            localSmoothedPoints = spline.SmoothedPoints.ToVector3().ToVector2();
        }
        protected override void GetLocalHandlePoints()
        {
            localHandlePoints = spline.AnchorPoints.ToVector3().ToVector2();
        }

        protected override float GetSplineLength()
        {
            return spline.TotalSplineLength;
        }

        protected override void SetPoint(int id, Vector2 pointValue)
        {

        }
        protected override void AddPoint(Vector2 point)
        {

        }

        protected override Color GetColor(float percent, Vector2 rawPosition)
        {
            return Color.white;
        }

        protected override Vector2 GetClosestPointOnSpline(Vector2 point, ref int closestAnchor)
        {
            return Vector2.zero;
        }
        protected override void MoveSpline(Vector2 moveValue)
        {
            throw new NotImplementedException();
        }
        protected override void RotateSpline(float degrees)
        {
            throw new NotImplementedException();
        }
        protected override void ScaleSpline(Vector2 newScale)
        {
            throw new NotImplementedException();
        }
    }
}