using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Formal.Mathematics
{
    

    public class Spline2DrawingElement : SplineDrawingElement<Spline2>
    {
        #region UXML Factory

        public new class UxmlFactory : UxmlFactory<Spline2DrawingElement, UxmlTraits> { }

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
                var splineElement = ve as Spline2DrawingElement;

                splineElement.LineWidth = lWDescription.GetValueFromBag(bag, cc);
            }
        }

        #endregion
         
        public Spline2DrawingElement() : base()
        {

        }

        public Spline2DrawingElement(Spline2 spline) :base()
        {
            this.spline = spline;
            splineCanvas.AddDrawData(CreateDrawData());

        }

        protected override void GetLocalSmoothedPoints()
        {
            //Vector2 offset = spline.Extents.Min.ToVector2();
            //offset *= -1;
            Vector2 size = spline.Extents.Size.ToVector2();

            localSmoothedPoints = spline.SmoothedPoints.ToVector2();
            for(int i =0; i < localSmoothedPoints.Length; i++)
            {
                Vector2 lsp = localSmoothedPoints[i]; // + offset;

                lsp = new Vector2(lsp.x * RenderScale, lsp.y * RenderScale);
                
                localSmoothedPoints[i] = lsp;
            }

        }
        protected override void GetLocalHandlePoints()
        {
            //Vector2 offset = spline.Extents.Min.ToVector2();
            //offset *= -1;

            localHandlePoints = spline.AnchorPoints.ToVector2();
            if (spline.wrap)
            {
                System.Array.Resize(ref localHandlePoints, localHandlePoints.Length - 1);
            }
            for (int i = 0; i < localHandlePoints.Length; i++)
            {
                Vector2 lsp = localHandlePoints[i]; // + offset;

                lsp = new Vector2(lsp.x * RenderScale,  lsp.y * RenderScale);

                localHandlePoints[i] = lsp;
            }
        }

        protected override float GetSplineLength()
        {
            return spline.TotalSplineLength;
        }

        protected override void SetPoint(int id, Vector2 pointValue)
        {
            //Vector2 offset = spline.Extents.Min.ToVector2();
            //offset *= -1;

            Vector2 unscaledPoint = pointValue;
            
            unscaledPoint = new Vector2(unscaledPoint.x*Unscaler, (unscaledPoint.y*Unscaler));
            //unscaledPoint -= offset;
            spline[id] = unscaledPoint.ToFloat2();

            RedrawSpline(false);
        }

        int closestAnchor = -1;
        
        protected override Vector2 GetClosestPointOnSpline(Vector2 point, ref int closestAnchor)
        {
            Vector2 unscaledPoint = point;

            unscaledPoint = new Vector2(unscaledPoint.x * Unscaler, unscaledPoint.y * Unscaler);
            Float2 closest = spline.GetClosestPointOnPolygon(unscaledPoint.ToFloat2(), 20, ref closestAnchor);
            this.closestAnchor = closestAnchor;
            return closest.ToVector2()*RenderScale;
        }
        protected override void AddPoint(Vector2 point)
        {
            Float2 p = point.ToFloat2();
            p *= Unscaler;

            spline.AddPoint(p, closestAnchor, CurveType.Linear);
            RedrawSpline();
        }

        protected override Color GetColor(float percent, Vector2 rawPosition)
        {
            if (selected)
            {
                return splineColor.GetPoint(percent).ToColor();
            }
            else { return Color.gray; }
            
        }

        protected override void PositionSplineToolHandle()
        {
            if(spline == null) { base.PositionSplineToolHandle(); return; }
            
            Vector2 scaledCenter = spline.AveragePoints().ToVector2();
            scaledCenter *= RenderScale;
            splineToolHandle.style.left = scaledCenter.x - halfHandle;
            splineToolHandle.style.top = scaledCenter.y - halfHandle;
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