using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Formal.Mathematics
{

    public class Spline1DrawingElement : SplineDrawingElement<Spline1>
    {
        #region UXML Factory

        public new class UxmlFactory : UxmlFactory<Spline1DrawingElement, UxmlTraits> { }

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
                var splineElement = ve as Spline1DrawingElement;

                splineElement.LineWidth = lWDescription.GetValueFromBag(bag, cc);
            }
        }

        #endregion

        public override event Action<int> OnSetPoint;

        public Spline1DrawingElement() : base()
        {
        }

        public Spline1DrawingElement(Spline1 spline) : base()
        {
            this.spline = spline;
            splineCanvas.AddDrawData(CreateDrawData());
        }

        protected override void GetLocalSmoothedPoints()
        {
            int[] smoothSections;

            float[] sp = spline.GetSmoothedPoints(out smoothSections).Tofloat();
            
            localSmoothedPoints = new Vector2[sp.Length];
            float xS = worldBound.width / spline.totalWeight;
            float yS = worldBound.height;
            int lastSection = 0;
            float sectionWeight = spline.GetSection(0).weight;
            float currentWeight = 0;

            for(int i = 1; i < smoothSections.Length; i++)
            {
                int numberOfPointsInSection = (smoothSections[i] - lastSection);
                float nextWeight = 0f;
                if (i < smoothSections.Length - 1) { nextWeight = spline.GetSection(i).weight; }

                float weightPerPoint = sectionWeight / numberOfPointsInSection;
                for(int j = lastSection; j <= smoothSections[i]; j++)
                {
                    localSmoothedPoints[j] = new Vector2(currentWeight * xS, yS - (sp[j] * yS));
                    currentWeight += weightPerPoint;
                }
                lastSection = smoothSections[i]+1;
                sectionWeight = nextWeight;
            }
        }

        protected override void GetLocalHandlePoints()
        {
            float[] sp = spline.AnchorPoints.Tofloat();
            localHandlePoints = new Vector2[sp.Length];

            float xS = worldBound.width/spline.totalWeight;
            float yS = worldBound.height;
            float tw = 0;
            for (int i = 0; i < sp.Length; i++)
            {
                localHandlePoints[i] = new Vector2(tw * xS, yS - (sp[i] * yS));
                tw += spline.GetSection(i).weight;
            }
        }

        protected override float GetSplineLength()
        {
            return spline.GetTotalSplineLength(0.05f);
        }

        protected override bool DragX { get { return false; } }

        protected override void SetPoint(int id, Vector2 pointValue)
        {
            float max = spline.Max;
            float finalValue = ((worldBound.height-pointValue.y)/worldBound.height);
            spline[id] = finalValue;
            RedrawSpline(false);
            OnSetPoint?.Invoke(id);
        }

        protected override void AddPoint(Vector2 point)
        {
            spline.InsertPoint(point.y, point.x);
            RedrawSpline();
        }

        protected override Color GetColor(float percent, Vector2 rawPosition)
        {
            Vector4 pointColor = Vector4.one;
            switch (colorSampling)
            {
                case SplineColorSampling.ByLength:
                    pointColor = splineColor.GetPoint(percent).ToVector4();
                    break;
                case SplineColorSampling.ByValue:
                    float v = (worldBound.height - rawPosition.y) / worldBound.height;
                    
                    pointColor = splineColor.GetPoint(v).ToVector4();
                    break;
            }

            Color color = Color.HSVToRGB(pointColor.x, pointColor.y, pointColor.z);
            color.a = pointColor.w;
            return color;
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
            return;
        }
        protected override void ScaleSpline(Vector2 newScale)
        {
            throw new NotImplementedException();
        }
    }
}