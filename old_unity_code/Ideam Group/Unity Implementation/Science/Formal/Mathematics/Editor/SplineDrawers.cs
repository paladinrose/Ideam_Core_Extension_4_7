using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;
using UnityEditor;

using Ideam.Unity.UI;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

using Quaternion = UnityEngine.Quaternion;


namespace Ideam.Unity.Science.Formal.Mathematics
{
    public static class SplineDrawers
    {
       
    }

    public abstract class SplineHandleDrawer<T, TPoint, TVector, TElement>
        where T:SplineBase
        where TElement:SplineBaseUI<T>
    {

        public virtual event Action<int> OnSelectPoint, OnSelectSpline;
        public virtual event Action OnStartPointMove,      OnPointMove,      OnStopPointMove, 
                                    OnStartForeHandleMove, OnForeHandleMove, OnStopForeHandleMove,
                                    OnStartBackHandleMove, OnBackHandleMove, OnStopBackHandleMove;

        protected Transform localizer;
        protected bool preserveLength;
        
        protected int currentPointID = -1, currentSplineID = -1;
        protected int[] currentSplineSelection, currentPointSelection;

        protected T[] splines, worldSplines;
        protected TElement[] splineElements;
        protected TPoint[] newPoints;
        protected TVector[] storedPoints, currentPoints, foreHandles, backHandles, smoothedPoints;

        public Color selectedPointColor = Color.white, unselectedPointColor = Color.gray, addPointColor = Color.green, removePointColor = Color.red;

        public virtual TElement[] SplineElements { get { return splineElements; } }

        public SplineHandleDrawer(bool preserveLength = false, Transform transform = null, VisualElement splineElementsParent = null)
        {
            this.preserveLength = preserveLength;
            localizer = transform;
        }

        public virtual void DrawSplineAndHandles()
        {
            if (currentSplineID < 0 || splines[currentSplineID].Length == 0) { return; }
            if (Event.current.shift)
            {
                DrawAddPointSpline();
            }
            else if (Event.current.alt)
            {
                DrawRemovePointSpline();
            }
            else
            {
                DrawSpline();
                Handles.color = Color.white;
                DrawHandles();
            }
        }
        
        public abstract void DrawSpline();
        public abstract void DrawAddPointSpline();
        public abstract void DrawRemovePointSpline();
        public abstract void DrawHandles();
        public abstract void DrawForeHandle(int id);
        public abstract void DrawBackHandle(int id);
        
        public virtual void SelectSpline(int id)
        {
            if(id == currentSplineID) { return; }
            if(id >= splines.Length || id < 0) { id = -1; }
            if(currentSplineID >= 0) { splineElements[currentSplineID].style.display = DisplayStyle.None; }
            currentSplineID = id;
            if(currentSplineID >= 0) { splineElements[currentSplineID].style.display = DisplayStyle.Flex; }
            OnSelectSpline?.Invoke(currentSplineID);
        }

        public virtual void SelectPoint(int id)
        {
            if(id== currentPointID) { return; }
            if(currentSplineID < 0 || id < 0 || id >= splines[currentSplineID].Length) { id = -1; }
            currentPointID = id;
            if(splineElements.Length == splines.Length && splineElements[currentSplineID] != null) splineElements[currentSplineID].SelectPoint(id);
            OnSelectPoint?.Invoke(currentPointID);
        }

        protected virtual void GetValuesFromSpline() { }
    }
    
    public class Spline2HandleDrawer : SplineHandleDrawer<Spline2, Point2, Vector3, Spline2Element>
    {
        public override event Action OnStartPointMove, OnStopPointMove;

        public Spline2HandleDrawer(Spline2[] splinesToDraw, bool preserveLength = false, Transform transform = null, VisualElement splineElementsParent = null) : base (preserveLength, transform, splineElementsParent)
        {
            splines = splinesToDraw;
            localizer = transform;
            currentSplineID = 0;

            GetValuesFromSpline();

            if(splineElementsParent != null)
            {
                splineElements = new Spline2Element[splines.Length];
                for(int i =0; i < splines.Length; i++)
                {
                    splineElements[i] = new Spline2Element(splines[i]);
                    splineElements[i].OnAxisConversion += AxisConversion;
                    splineElements[i].OnSelectPoint += SelectPoint;
                    splineElements[i].OnSectionCurveChange += (id, curveType) => { GetValuesFromSpline(); };
                    splineElements[i].OnSectionWeightChange += (id, weight) => { GetValuesFromSpline(); };
                    splineElementsParent.Add(splineElements[i]);
                }
            }

            storedPoints = currentPoints;
            currentPointID = -1;
            this.preserveLength = preserveLength;
        }

        protected override void GetValuesFromSpline()
        {
            Spline2 spline = splines[0];

            currentPoints = spline.points.GetPoints().ToVector3(spline.axisConversion);

            newPoints = spline.points;

            foreHandles = spline.points.GetForeHandles().ToVector3(spline.axisConversion);
            backHandles = spline.points.GetBackHandles().ToVector3(spline.axisConversion);
            smoothedPoints = spline.SmoothedPoints.ToVector3(spline.axisConversion);

            if (localizer)
            {
                currentPoints = localizer.TransformPoints(currentPoints);
                smoothedPoints = localizer.TransformPoints(smoothedPoints);

                foreHandles = localizer.TransformPoints(foreHandles);
                backHandles = localizer.TransformPoints(backHandles);

                for (int i = 0; i < currentPoints.Length; i++)
                {
                    foreHandles[i] += currentPoints[i];
                    backHandles[i] += currentPoints[i];
                }
            }
        }

        private void AxisConversion(XYToXYZ newAxisConversion)
        {
            Spline2 spline = splines[currentSplineID];

            currentPoints = spline.points.GetPoints().ToVector3(spline.axisConversion);

            newPoints = spline.points;

            foreHandles = spline.points.GetForeHandles().ToVector3(spline.axisConversion);
            backHandles = spline.points.GetBackHandles().ToVector3(spline.axisConversion);
            smoothedPoints = spline.SmoothedPoints.ToVector3(spline.axisConversion);
        }

        public override void DrawSpline()
        {
            Spline2 spline = splines[currentSplineID];

            Color c = Color.red;
            float s = 1f / smoothedPoints.Length;
            for (int i = 0; i < smoothedPoints.Length - 1; i++)
            {
                float p = s * i;
                c = Color.Lerp(Color.red, Color.green, p);
                Handles.color = c;
                Handles.DrawLine(smoothedPoints[i], smoothedPoints[i + 1]);
            }
            if (spline.Wrap) { c = Color.green; Handles.color = c; Handles.DrawLine(smoothedPoints[smoothedPoints.Length - 1], smoothedPoints[0]); }
        }

        public override void DrawHandles()
        {
            Spline2 spline = splines[currentSplineID];
            int cpl = currentPoints.Length; if (spline.Wrap) { cpl--; }
            for (int i = 0; i < cpl; i++)
            {
                if (i != currentPointID)
                {
                    float hs = HandleUtility.GetHandleSize(currentPoints[i]) * 0.05f;
                    if (Handles.Button(currentPoints[i], Quaternion.identity, hs, hs * 1.2f, Handles.CubeHandleCap))
                    {
                        currentPointID = i;
                        splineElements[currentSplineID].SelectPoint(currentPointID);
                    }
                }
                else
                {
                    Vector3 currentPos = Handles.PositionHandle(currentPoints[i], Quaternion.identity);

                    if (currentPos == currentPoints[i])
                    {
                        if (Event.current.type == EventType.MouseUp)
                        {
                            OnStopPointMove?.Invoke();
                        }
                        else if (Event.current.type == EventType.MouseDown)
                        {
                            OnStartPointMove?.Invoke();
                        }
                        continue;
                    }

                    if (preserveLength)
                    {
                        Vector3 moveOffset = Vector3.zero;

                        if (i > 0)
                        {
                            float originalDistance = Vector3.Distance(storedPoints[i - 1], storedPoints[i]);
                            Vector3 currentDirection = (currentPos - currentPoints[i - 1]).normalized;
                            currentPos = currentPoints[i - 1] + (currentDirection * originalDistance);
                            moveOffset = currentPos - currentPoints[i];
                            for (int j = i; j < currentPoints.Length; j++)
                            {
                                currentPoints[j] += moveOffset;
                                if (localizer)
                                {
                                    newPoints[j] = new Point2()
                                    {
                                        point = localizer.InverseTransformPoint(currentPoints[j]).ToFloat2(spline.axisConversion),
                                        backHandle = backHandles[j].ToFloat2(spline.axisConversion),
                                        foreHandle = foreHandles[j].ToFloat2(spline.axisConversion)
                                    };
                                }
                                else
                                {
                                    newPoints[j] = new Point2()
                                    {
                                        point = currentPoints[j].ToFloat2(spline.axisConversion),
                                        backHandle = backHandles[j].ToFloat2(spline.axisConversion),
                                        foreHandle = foreHandles[j].ToFloat2(spline.axisConversion)
                                    };
                                }

                            }
                        }
                        else
                        {
                            float originalDistance = Vector3.Distance(storedPoints[1], storedPoints[0]);
                            Vector3 currentDirection = (currentPoints[1] - currentPos).normalized;
                            currentPoints[i] = currentPoints[1] + (currentDirection * originalDistance);
                            if (localizer)
                            {
                                newPoints[i] = new Point2()
                                {
                                    point = localizer.InverseTransformPoint(currentPoints[i]).ToFloat2(spline.axisConversion),
                                    backHandle = backHandles[i].ToFloat2(spline.axisConversion),
                                    foreHandle = foreHandles[i].ToFloat2(spline.axisConversion)
                                };
                            }
                            else
                            {
                                newPoints[i] = new Point2()
                                {
                                    point = currentPoints[i].ToFloat2(spline.axisConversion),
                                    backHandle = backHandles[i].ToFloat2(spline.axisConversion),
                                    foreHandle = foreHandles[i].ToFloat2(spline.axisConversion)
                                };
                            }
                        }
                    }
                    else
                    {
                        currentPoints[i] = currentPos;
                        if (localizer)
                        {
                            newPoints[i] = new Point2()
                            {
                                point = localizer.InverseTransformPoint(currentPoints[i]).ToFloat2(spline.axisConversion),
                                backHandle = backHandles[i].ToFloat2(spline.axisConversion),
                                foreHandle = foreHandles[i].ToFloat2(spline.axisConversion)
                            };
                        }
                        else
                        {
                            newPoints[i] = new Point2()
                            {
                                point = currentPoints[i].ToFloat2(spline.axisConversion),
                                backHandle = backHandles[i].ToFloat2(spline.axisConversion),
                                foreHandle = foreHandles[i].ToFloat2(spline.axisConversion)
                            };
                        }
                    }
                    if (spline.Wrap)
                    {
                        newPoints[newPoints.Length - 1] = newPoints[0];
                    }
                    spline.SetAnchorPoints(newPoints);

                    if (localizer)
                    {
                        smoothedPoints = localizer.TransformPoints(spline.SmoothedPoints.ToVector3(spline.axisConversion));
                    }
                    else
                    {
                        smoothedPoints = spline.SmoothedPoints.ToVector3(spline.axisConversion);
                    }

                    splineElements[currentSplineID].UpdatePointFields();

                    if (Event.current.type == EventType.MouseUp)
                    {

                        OnStopPointMove?.Invoke();

                    }
                }
            }
        }

        public override void DrawForeHandle(int id)
        {

        }

        public override void DrawBackHandle(int id)
        {

        }

        public override void DrawAddPointSpline()
        {
            
            Spline2 spline = splines[currentSplineID];

            Vector3 mousePosition = Event.current.mousePosition;
            Ray ray = HandleUtility.GUIPointToWorldRay(mousePosition);
            Plane splinePlane;

            Float2 splineMousePos = Float2.zero;
            if (localizer)
            {
                splinePlane = new Plane(localizer.up, localizer.position);
                if(splinePlane.Raycast(ray, out float hitLength))
                {
                    Vector3 hitPoint = ray.GetPoint(hitLength);
                    splineMousePos = localizer.InverseTransformPoint(hitPoint).ToFloat2(spline.axisConversion);
                }
                
            }
            int closestSection = -1;

            Float2 splineHandlePoint = spline.GetClosestPointOnPolygon(splineMousePos, 20, ref closestSection);

            Vector3 handlePoint = splineHandlePoint.ToVector3(spline.axisConversion);
            if (localizer)
            {
                handlePoint = localizer.TransformPoint(handlePoint);
            }
            float hs = HandleUtility.GetHandleSize(handlePoint) * 0.05f;
            
            int nextSection = closestSection + 1;
            if(nextSection > spline.Sections) { nextSection = 0; }
            
            int closestSectionSmoothStart = spline.newIDS[closestSection], 
                closesSectionSmoothEnd = spline.newIDS[nextSection];

            for (int i = 0; i < smoothedPoints.Length-1; i++)
            {
                if(i < closestSectionSmoothStart || i > closesSectionSmoothEnd) { Handles.color = unselectedPointColor; }
                else { Handles.color = addPointColor; }
                Handles.DrawLine(smoothedPoints[i], smoothedPoints[i + 1]);
            }

            if(Handles.Button(handlePoint, Quaternion.identity, hs, hs * 1.2f, Handles.CubeHandleCap))
            {
                SplineSection sec = spline.sections[closestSection];
                float lengthFromClosestToNewPoint = Float2.Distance(spline[closestSection], splineHandlePoint);
               
                float newWeight = sec.weight * lengthFromClosestToNewPoint / sec.length;
                sec.weight -= newWeight;
                
                spline.sections[closestSection] = sec;

                spline.AddPoint(splineHandlePoint, nextSection, spline.sections[closestSection].curveType, newWeight);
            }
        }

        public override void DrawRemovePointSpline()
        {
            // Find the closest anchor point to the mouse.
            Spline2 spline = splines[currentSplineID];

            Vector3 mousePosition = Event.current.mousePosition;
            Ray ray = HandleUtility.GUIPointToWorldRay(mousePosition);
            Plane splinePlane;

            Float2 splineMousePos = Float2.zero;
            if (localizer)
            {
                splinePlane = new Plane(localizer.up, localizer.position);
                if (splinePlane.Raycast(ray, out float hitLength))
                {
                    Vector3 hitPoint = ray.GetPoint(hitLength);
                    splineMousePos = localizer.InverseTransformPoint(hitPoint).ToFloat2(spline.axisConversion);
                }

            }
            int closestSection = spline.GetClosestAnchorPoint(splineMousePos);

            // Draw the spline, as above, but the section BEFORE and AFTER should draw their lines with removeColor;
            int nextSection = closestSection + 1;
            if (nextSection > spline.Sections) { nextSection = 0; }
            //We move closest section BACK by one, now.
            int previousSection = closestSection - 1;
            
            if(previousSection < 0) { previousSection = spline.Sections - 1; }

            int closestSectionSmoothStart = spline.newIDS[previousSection],
                closesSectionSmoothEnd = spline.newIDS[nextSection];

            if (closesSectionSmoothEnd < closestSectionSmoothStart)
            {

            }
            else
            {
                for (int i = 0; i < smoothedPoints.Length - 1; i++)
                {
                    if (i < closestSectionSmoothStart || i > closesSectionSmoothEnd) { Handles.color = unselectedPointColor; }
                    else { Handles.color = addPointColor; }
                    Handles.DrawLine(smoothedPoints[i], smoothedPoints[i + 1]);
                }
            }
            // Draw all handles, as in DrawHandles, but with the closest point being in removeColor, and all others in unselectedColor.

            int cpl = currentPoints.Length; if (spline.Wrap) { cpl--; }
            for (int i = 0; i < cpl; i++)
            {
                if (i != closestSection)
                {
                    float hs = HandleUtility.GetHandleSize(currentPoints[i]) * 0.05f;
                    Handles.color = unselectedPointColor;
                    Handles.Button(currentPoints[i], Quaternion.identity, hs, hs * 1.2f, Handles.CubeHandleCap);
                }
                else
                {
                    float hs = HandleUtility.GetHandleSize(currentPoints[i]) * 0.05f;
                    Handles.color = removePointColor;
                    if (Handles.Button(currentPoints[i], Quaternion.identity, hs, hs * 1.2f, Handles.CubeHandleCap))
                    {
                        spline.RemovePoint(i);
                    }
                }
            }
        }

    }

    public class Spline3HandleDrawer :SplineHandleDrawer<Spline3,Point3,Vector3, Spline3Element>
    {
        public override event Action OnStartPointMove, OnStopPointMove;

        public Spline3HandleDrawer(Spline3[] splinesToDraw, bool preserveLength = false, Transform transform = null, VisualElement splineElementsParent = null) :base(preserveLength, transform, splineElementsParent)
        {
            splines = splinesToDraw;
            localizer = transform;
            currentSplineID = 0;
            
            Spline3 spline = splinesToDraw[0];

            currentPoints = spline.points.GetPoints().ToVector3();
            
            newPoints = spline.points;

            foreHandles = spline.points.GetForeHandles().ToVector3();
            backHandles = spline.points.GetBackHandles().ToVector3();
            smoothedPoints = spline.SmoothedPoints.ToVector3();

            if (localizer)
            {
                currentPoints = localizer.TransformPoints(currentPoints);
                smoothedPoints = localizer.TransformPoints(smoothedPoints);
                foreHandles = localizer.TransformPoints(foreHandles);
                backHandles = localizer.TransformPoints(backHandles);
                for(int i =0; i < currentPoints.Length; i++)
                {
                    foreHandles[i] += currentPoints[i];
                    backHandles[i] += currentPoints[i];
                }

                
            }

            if (splineElementsParent != null)
            {
                splineElements = new Spline3Element[splines.Length];
                for (int i = 0; i < splines.Length; i++)
                {
                    splineElements[i] = new Spline3Element(splines[i]);

                    splineElements[i].OnSelectPoint += SelectPoint;
                    splineElementsParent.Add(splineElements[i]);
                }
            }

            storedPoints = currentPoints;
            currentPointID = -1;
            this.preserveLength = preserveLength;
        }

       
        public override void DrawSpline()
        {
            Spline3 spline = splines[currentSplineID];

            Color c = Color.red;
            float s = 1f / smoothedPoints.Length;
            for(int i =0; i < smoothedPoints.Length-1; i++)
            {
                float p = s * i;
                c = Color.Lerp(Color.red, Color.green, p);
                Handles.color = c;
                Handles.DrawLine(smoothedPoints[i], smoothedPoints[i + 1]);
            }
            if (spline.Wrap) { c = Color.green; Handles.color = c; Handles.DrawLine(smoothedPoints[smoothedPoints.Length - 1], smoothedPoints[0]); }
        }

        public override void DrawHandles()
        {
            Spline3 spline = splines[currentSplineID];

            for(int i =0; i < currentPoints.Length; i++)
            {
                if(i != currentPointID)
                {
                    float hs = HandleUtility.GetHandleSize(currentPoints[i])*0.05f;
                    if(Handles.Button(currentPoints[i], Quaternion.identity, hs, hs*1.2f, Handles.CubeHandleCap))
                    {
                        currentPointID = i;
                        
                    }
                    
                }
                else
                {
                    Vector3 currentPos = Handles.PositionHandle(currentPoints[i], Quaternion.identity);

                    if (currentPos == currentPoints[i])
                    {
                        if (Event.current.type == EventType.MouseUp)
                        {

                            OnStopPointMove?.Invoke();

                        }
                        continue;
                    }

                    if (preserveLength)
                    {
                        Vector3 moveOffset = Vector3.zero;

                        if(i > 0) 
                        {
                            float originalDistance = Vector3.Distance(storedPoints[i - 1], storedPoints[i]);
                            Vector3 currentDirection = (currentPos - currentPoints[i - 1]).normalized;
                            currentPos = currentPoints[i - 1] + (currentDirection * originalDistance);
                            moveOffset = currentPos - currentPoints[i];
                            for(int j = i; j < currentPoints.Length; j++)
                            {
                                currentPoints[j] += moveOffset;
                                if (localizer)
                                {
                                    newPoints[j] = new Point3()
                                    {
                                        point = localizer.InverseTransformPoint(currentPoints[j]).ToFloat3(),
                                        backHandle = backHandles[j].ToFloat3(),
                                        foreHandle = foreHandles[j].ToFloat3()
                                    };
                                }
                                else
                                {
                                    newPoints[j] = new Point3()
                                    {
                                        point = currentPoints[j].ToFloat3(),
                                        backHandle = backHandles[j].ToFloat3(),
                                        foreHandle = foreHandles[j].ToFloat3()
                                    };
                                }
                                
                            }
                        }
                        else 
                        { 
                            float originalDistance = Vector3.Distance(storedPoints[1], storedPoints[0]);
                            Vector3 currentDirection = (currentPoints[1] - currentPos).normalized;
                            currentPoints[i] = currentPoints[1] + (currentDirection * originalDistance);
                            if (localizer)
                            {
                                newPoints[i] = new Point3()
                                {
                                    point = localizer.InverseTransformPoint(currentPoints[i]).ToFloat3(),
                                    backHandle = backHandles[i].ToFloat3(),
                                    foreHandle = foreHandles[i].ToFloat3()
                                };
                            }
                            else
                            {
                                newPoints[i] = new Point3()
                                {
                                    point = currentPoints[i].ToFloat3(),
                                    backHandle = backHandles[i].ToFloat3(),
                                    foreHandle = foreHandles[i].ToFloat3()
                                };
                            }
                        }
                    }
                    else
                    {
                        currentPoints[i] = currentPos;
                        if (localizer)
                        {
                            newPoints[i] = new Point3()
                            {
                                point = localizer.InverseTransformPoint(currentPoints[i]).ToFloat3(),
                                backHandle = backHandles[i].ToFloat3(),
                                foreHandle = foreHandles[i].ToFloat3()
                            };
                        }
                        else
                        {
                            newPoints[i] = new Point3()
                            {
                                point = currentPoints[i].ToFloat3(),
                                backHandle = backHandles[i].ToFloat3(),
                                foreHandle = foreHandles[i].ToFloat3()
                            };
                        }
                    }

                    spline.SetAnchorPoints(newPoints);
                    if (localizer)
                    {
                        smoothedPoints = localizer.TransformPoints( spline.SmoothedPoints.ToVector3());
                    }
                    else
                    {
                        smoothedPoints = spline.SmoothedPoints.ToVector3();
                    }
                    if (Event.current.type == EventType.MouseUp)
                    {

                        OnStopPointMove?.Invoke();

                    }
                }
            }
        }
        
        public override void DrawForeHandle(int id)
        {

        }

        public override void DrawBackHandle(int id) 
        {

        }

        public override void DrawAddPointSpline()
        {

            Spline3 spline = splines[currentSplineID];

            Vector3 centerPoint = spline.Extents.Center.ToVector3();
            if (localizer)
            {
                centerPoint = localizer.TransformPoint(centerPoint);
            }
            float distFromCamera = Vector3.Distance(centerPoint, SceneView.currentDrawingSceneView.camera.transform.position);

            Vector3 mousePosition = Event.current.mousePosition;
            Ray ray = HandleUtility.GUIPointToWorldRay(mousePosition);
            mousePosition = ray.GetPoint(distFromCamera);
            if (localizer)
            {
                mousePosition = localizer.InverseTransformPoint(mousePosition);
            }

            int closestSection = -1;
            Float3 splineHandlePoint = spline.GetClosestPointOnSpline(mousePosition.ToFloat3(), 20, ref closestSection);

            Vector3 handlePoint = splineHandlePoint.ToVector3();
            if (localizer)
            {
                handlePoint = localizer.TransformPoint(handlePoint);
            }
            float hs = HandleUtility.GetHandleSize(handlePoint) * 0.05f;

            int nextSection = closestSection + 1;
            if (nextSection > spline.Sections) { nextSection = 0; }

            int closestSectionSmoothStart = spline.newIDS[closestSection],
                closesSectionSmoothEnd = spline.newIDS[nextSection];

            for (int i = 0; i < smoothedPoints.Length - 1; i++)
            {
                if (i < closestSectionSmoothStart || i > closesSectionSmoothEnd) { Handles.color = unselectedPointColor; }
                else { Handles.color = addPointColor; }
                Handles.DrawLine(smoothedPoints[i], smoothedPoints[i + 1]);
            }

            if (Handles.Button(handlePoint, Quaternion.identity, hs, hs * 1.2f, Handles.CubeHandleCap))
            {
                SplineSection sec = spline.sections[closestSection];
                float lengthFromClosestToNewPoint = Float3.Distance(spline[closestSection], splineHandlePoint);

                float newWeight = sec.weight * lengthFromClosestToNewPoint / sec.length;
                sec.weight -= newWeight;

                spline.sections[closestSection] = sec;

                spline.AddPoint(splineHandlePoint, nextSection, spline.sections[closestSection].curveType, newWeight);
            }
        }

        public override void DrawRemovePointSpline()
        {
            // Find the closest anchor point to the mouse.
            Spline3 spline = splines[currentSplineID];
            
            Vector3 centerPoint = spline.Extents.Center.ToVector3();
            if (localizer)
            {
                centerPoint = localizer.TransformPoint(centerPoint);
            }
            float distFromCamera = Vector3.Distance(centerPoint, SceneView.currentDrawingSceneView.camera.transform.position);

            Vector3 mousePosition = Event.current.mousePosition;
            Ray ray = HandleUtility.GUIPointToWorldRay(mousePosition);
            mousePosition = ray.GetPoint(distFromCamera);
            if (localizer)
            {
                mousePosition = localizer.InverseTransformPoint(mousePosition);
            }

            int closestSection = spline.GetClosestAnchorPoint(mousePosition.ToFloat3());

            // Draw the spline, as above, but the section BEFORE and AFTER should draw their lines with removeColor;
            int nextSection = closestSection + 1;
            if (nextSection > spline.Sections) { nextSection = 0; }
            //We move closest section BACK by one, now.
            int previousSection = closestSection - 1;

            if (previousSection < 0) { previousSection = spline.Sections - 1; }

            int closestSectionSmoothStart = spline.newIDS[previousSection],
                closesSectionSmoothEnd = spline.newIDS[nextSection];

            if (closesSectionSmoothEnd < closestSectionSmoothStart)
            {

            }
            else
            {
                for (int i = 0; i < smoothedPoints.Length - 1; i++)
                {
                    if (i < closestSectionSmoothStart || i > closesSectionSmoothEnd) { Handles.color = unselectedPointColor; }
                    else { Handles.color = addPointColor; }
                    Handles.DrawLine(smoothedPoints[i], smoothedPoints[i + 1]);
                }
            }
            // Draw all handles, as in DrawHandles, but with the closest point being in removeColor, and all others in unselectedColor.

            int cpl = currentPoints.Length; if (spline.Wrap) { cpl--; }
            for (int i = 0; i < cpl; i++)
            {
                if (i != closestSection)
                {
                    float hs = HandleUtility.GetHandleSize(currentPoints[i]) * 0.05f;
                    Handles.color = unselectedPointColor;
                    Handles.Button(currentPoints[i], Quaternion.identity, hs, hs * 1.2f, Handles.CubeHandleCap);
                }
                else
                {
                    float hs = HandleUtility.GetHandleSize(currentPoints[i]) * 0.05f;
                    Handles.color = removePointColor;
                    if (Handles.Button(currentPoints[i], Quaternion.identity, hs, hs * 1.2f, Handles.CubeHandleCap))
                    {
                        spline.RemovePoint(i);
                    }
                }
            }

        }
    }
}