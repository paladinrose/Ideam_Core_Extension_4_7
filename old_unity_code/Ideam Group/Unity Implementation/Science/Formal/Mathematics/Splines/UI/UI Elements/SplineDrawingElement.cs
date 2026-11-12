using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Formal.Mathematics
{
    public abstract class SplineDrawingElement<T> : VisualElement
        where T : SplineBase
    {

        public SplineDrawingElement() : base()
        {
            style.position = Position.Absolute;
            style.left = 0; style.right = 0;
            style.top = 0; style.bottom = 0;

            Float4[] gradVals = new Color[]
            {
                Color.black, Color.red, Color.yellow, Color.green, Color.cyan, Color.blue, Color.white
            }.ToFloat4();
            
            splineColor = Spline4.Gradient(gradVals);

            colorSampling = SplineColorSampling.ByValue;

            splineCanvas = new DrawingElement() { name = "SplineCanvas" };
            splineCanvas.style.position = Position.Absolute;
            splineCanvas.style.left = 0; splineCanvas.style.right = 0;
            splineCanvas.style.top = 0; splineCanvas.style.bottom = 0;
            splineCanvas.AddToClassList("transparent");
            splineCanvas.focusable = false;
            Add(splineCanvas);

            
            handlePane = new VisualElement() { name = "HandlePane" };
            handlePane.style.position = Position.Absolute;
            handlePane.style.left = 0; handlePane.style.right = 0;
            handlePane.style.top = 0; handlePane.style.bottom = 0;
            handlePane.AddToClassList("transparent");
            Add(handlePane);


            halfHandle = handleSize * 0.5f;

            newHandle = new VisualElement();
            handlePane.Add(newHandle);
            newHandle.style.backgroundColor = Color.green;
            newHandle.style.height = handleSize;
            newHandle.style.width = handleSize;
            newHandle.style.position = Position.Absolute;
            newHandle.focusable = false;
            newHandle.style.display = DisplayStyle.None;

            primarySplineManipulator = new SplineManipulator()
            {
                splineElement = this,
                newHandle = newHandle,
                halfHandle = halfHandle
            };

            handlePane.AddManipulator(primarySplineManipulator);


            splineToolHandle = new VisualElement();
            splineToolHandle.style.backgroundColor = Color.white;
            splineToolHandle.style.height = handleSize;
            splineToolHandle.style.width = handleSize;
            splineToolHandle.style.position = Position.Absolute;

            PositionSplineToolHandle();
            Add(splineToolHandle);

            splineToolManipulator = new SplineToolManipulator()
            {
                splineElement = this,
                halfHandle = halfHandle
            };

            splineToolHandle.AddManipulator(splineToolManipulator);

            RegisterCallback<GeometryChangedEvent>(evt => { RedrawSpline(); });
        }

        protected SplineManipulator primarySplineManipulator;
        protected SplineToolManipulator splineToolManipulator;

        protected float halfHandle;

        protected bool mouseOver = false;


        [SerializeField]
        protected float lineWidth = 3f;
        public float LineWidth { get { return lineWidth; } set { lineWidth = value; } }

        [SerializeField]
        protected float renderScale = 10f, unscaler = 0.1f;
        public float RenderScale { get { return renderScale; } set { renderScale = value;  unscaler = 1f / renderScale; RedrawSpline(); } }
        public float Unscaler { get { return unscaler; } }

        [SerializeField]
        protected float handleSize = 10f;
        public float HandleSize { get { return handleSize; } set { handleSize = value; } }

        [SerializeField]
        protected T spline;
        public virtual T Spline { get { return spline; } set { spline = value; RedrawSpline(); } }

        [SerializeField]
        protected Spline4 splineColor;
        public Spline4 SplineColor { get { return splineColor; } set { splineColor = value; splineCanvas.MarkDirtyRepaint(); } }

        protected bool showHandles = true, didShowHandles = true;
        public bool ShowHandles { get { return showHandles; } 
            set 
            {
                if (value != showHandles) 
                {
                    if (value)
                    {
                        handlePane.style.display = DisplayStyle.Flex;
                    }
                    else
                    {
                        handlePane.style.display = DisplayStyle.None;
                    }
                    showHandles = value;
                }
            }
        }

        public enum SplineTool
        {
            None,
            Select,
            Move,
            Rotate,
            Scale
        }

        protected SplineTool splineTool;
        public SplineTool Tool { get { return splineTool; } 
            set 
            {
                if(value != splineTool)
                {
                    if (value == SplineTool.None) { splineToolHandle.style.display = DisplayStyle.None; }
                    else
                    {
                        splineToolHandle.style.display = DisplayStyle.Flex;
                        switch (splineTool)
                        {
                            case SplineTool.Select:
                                splineToolHandle.RemoveFromClassList("selectTool");
                                break;
                            case SplineTool.Move:
                                splineToolHandle.RemoveFromClassList("moveTool");
                                break;
                            case SplineTool.Rotate:
                                splineToolHandle.RemoveFromClassList("rotateTool");
                                break;
                            case SplineTool.Scale:
                                splineToolHandle.RemoveFromClassList("scaleTool");
                                break;
                        }
                        switch (value)
                        {
                            case SplineTool.Select:
                                splineToolHandle.AddToClassList("selectTool");
                                break;
                            case SplineTool.Move:
                                splineToolHandle.AddToClassList("moveTool");
                                break;
                            case SplineTool.Rotate:
                                splineToolHandle.AddToClassList("rotateTool");
                                break;
                            case SplineTool.Scale:
                                splineToolHandle.AddToClassList("scaleTool");
                                break;
                        }
                    }
                }
            }
        }

        public enum SplineColorSampling
        {
            ByLength,
            ByValue
        }

        [SerializeField]
        protected SplineColorSampling colorSampling;
        public SplineColorSampling ColorSampling
        {
            get { return colorSampling; } set { colorSampling = value; splineCanvas.MarkDirtyRepaint(); }
        }

        protected bool selected = false;
        public bool Selected { get { return selected; } 
            set 
            { 
                if (value != selected)
                {
                    if (value)
                    {
                        ShowHandles = didShowHandles;
                    }
                    else
                    {
                        didShowHandles = showHandles;
                        ShowHandles = false;
                    }
                    selected = value;
                    RedrawSpline();
                }
            } 
        }




        public virtual event Action<int> OnSetPoint;
        protected Vector2[] localHandlePoints, localSmoothedPoints;

        protected DrawingElement splineCanvas;
        protected VisualElement handlePane, newHandle, splineToolHandle;
        protected VisualElement[] pointHandles;

        public virtual void RedrawSpline(bool updateHandles = true)
        {
            splineCanvas.SetDrawData(CreateDrawData(), 0);
            splineCanvas.Redraw();

            PositionSplineToolHandle();

            if (!updateHandles) { return; }

            handlePane.MarkDirtyRepaint();
            CreatePointHandles();
        }

        protected virtual void PositionSplineToolHandle()
        {
            splineToolHandle.style.left = worldBound.center.x - halfHandle;
            splineToolHandle.style.top = worldBound.center.y - halfHandle;
        }

        protected virtual DrawData CreateDrawData()
        {
            DrawData drawData = new DrawData();
            if (lineWidth <= 0) return drawData;

            GetLocalSmoothedPoints();

            if (localSmoothedPoints.Length == 0) return drawData;

            uint pointCount = (uint)localSmoothedPoints.Length;
            uint wantedLength = (pointCount) * 2;
            uint indexCount = (wantedLength - 2) * 3;

            float halfWidth = lineWidth * 0.5f;

            Vector2 previousDirection = Vector2.zero;
            float colorPercent = 1f / pointCount;

            List<Vertex> newVerts = new List<Vertex>();
            List<int> newTris = new List<int>();

            for (int i = 0; i < pointCount; ++i)
            {
                Vector2 dir;
                Vector2 direction = Vector2.zero;
                Vector2 nextSegment = Vector2.zero;

                if (i < pointCount - 1)
                {
                    nextSegment = (localSmoothedPoints[i + 1] - localSmoothedPoints[i]);
                    direction = nextSegment.normalized;
                }


                if (i > 0 && i < pointCount - 1)
                {
                    dir = previousDirection + direction;
                    dir.Normalize();
                }
                else if (i > 0)
                {
                    dir = previousDirection;
                }
                else
                {
                    dir = direction;
                }

                Vector2 pos = localSmoothedPoints[i];
                Vector3 posA = new Vector3(pos.x + (dir.y * halfWidth), pos.y + (-dir.x * halfWidth), Vertex.nearZ);
                Vector3 posB = new Vector3(pos.x - (dir.y * halfWidth), pos.y - (-dir.x * halfWidth), Vertex.nearZ);

                Color color = GetColor(colorPercent * i, pos);

                newVerts.Add(new Vertex() { position = posA, tint = color });
                newVerts.Add(new Vertex() { position = posB, tint = color });
                
                previousDirection = direction;
            }

            // Fill triangle indices as it is a triangle strip
            for (int i = 0; i < wantedLength - 2; ++i)
            {
                if ((i & 0x01) == 0)
                {
                    newTris.Add(i);
                    newTris.Add(i + 2);
                    newTris.Add(i + 1);
                }
                else
                {
                    newTris.Add(i);
                    newTris.Add(i + 1);
                    newTris.Add(i + 2);
                }
            }

            drawData.Vertices = newVerts.ToArray();
            drawData.Triangles = newTris.ToArray();

            return drawData;
        }

        

        protected abstract Color GetColor(float percent, Vector2 rawPosition);
        protected abstract void GetLocalSmoothedPoints();
        protected abstract void GetLocalHandlePoints();
        protected abstract void SetPoint(int id, Vector2 pointValue);
        protected abstract void AddPoint(Vector2 point);
        protected abstract Vector2 GetClosestPointOnSpline(Vector2 point, ref int closestAnchor);
        protected abstract void MoveSpline(Vector2 moveValue);
        protected abstract void RotateSpline(float degrees);
        protected abstract void ScaleSpline(Vector2 newScale);
        
        protected virtual float GetSplineLength() { return spline.GetTotalSplineLength(0.1f); }

        #region Point Handles
        int draggingID = -1;
        protected virtual bool DragX { get { return true; } }
        protected virtual bool DragY { get { return true; } }

        protected virtual void CreatePointHandles()
        {
            handlePane.Clear();

            newHandle = new VisualElement();
            handlePane.Add(newHandle);
            newHandle.style.backgroundColor = Color.green;
            newHandle.style.height = handleSize;
            newHandle.style.width = handleSize;
            newHandle.style.position = Position.Absolute;
            newHandle.focusable = false;
            newHandle.style.display = DisplayStyle.None;
            primarySplineManipulator.newHandle = newHandle;

            GetLocalHandlePoints();

            pointHandles = new VisualElement[localHandlePoints.Length];

            float halfHandle = handleSize * 0.5f;

            for (int i = 0; i < localHandlePoints.Length; i++)
            {
                int id = i;

                VisualElement pointHandle = new VisualElement();
                pointHandle.style.backgroundColor = Color.white;
                pointHandle.style.height = handleSize;
                pointHandle.style.width = handleSize;
                pointHandle.style.position = Position.Absolute;
                pointHandle.style.left = localHandlePoints[id].x - halfHandle;
                pointHandle.style.top = localHandlePoints[id].y - halfHandle;

                SplineHandle sh = new SplineHandle()
                {
                    halfHandle = halfHandle,
                    dragX = DragX,
                    dragY = DragY,
                    splineElement = this
                };
                sh.OnHandleUpdate += newPoint => { SetPoint(id, newPoint); };
                pointHandle.AddManipulator(sh);
                
                pointHandles[id] = pointHandle;
                handlePane.Add(pointHandle);
            }
        }

        #endregion

        #region Spline Tools
        public class SplineToolManipulator : MouseManipulator

        {
            #region Init
            public SplineDrawingElement<T> splineElement;
            public event Action<Vector2> OnMove,  OnScale;
            public event Action<float> OnRotate;
            public float halfHandle;
            public bool dragX, dragY;
            private Vector2 m_Start;
            protected bool m_Active;

            public SplineToolManipulator()
            {
                activators.Add(new ManipulatorActivationFilter { button = MouseButton.LeftMouse });
                m_Active = false;
            }
            #endregion

            #region Registrations
            protected override void RegisterCallbacksOnTarget()
            {
                target.RegisterCallback<MouseDownEvent>(OnMouseDown);
                target.RegisterCallback<MouseMoveEvent>(OnMouseMove);
                target.RegisterCallback<MouseUpEvent>(OnMouseUp);
            }

            protected override void UnregisterCallbacksFromTarget()
            {
                target.UnregisterCallback<MouseDownEvent>(OnMouseDown);
                target.UnregisterCallback<MouseMoveEvent>(OnMouseMove);
                target.UnregisterCallback<MouseUpEvent>(OnMouseUp);
            }
            #endregion

            #region OnMouseDown
            protected virtual void OnMouseDown(MouseDownEvent e)
            {
                if (m_Active)
                {
                    e.StopImmediatePropagation();
                    return;
                }

                if (CanStartManipulation(e))
                {
                    m_Start = e.localMousePosition;

                    m_Active = true;
                    target.CaptureMouse();
                    e.StopPropagation();
                }
            }
            #endregion

            #region OnMouseMove
            protected virtual void OnMouseMove(MouseMoveEvent e)
            {
                if (!m_Active || !target.HasMouseCapture()) return;


                switch (splineElement.Tool)
                {
                    case SplineTool.Move:
                        splineElement.MoveSpline(e.mouseDelta);
                        OnMove?.Invoke(e.mouseDelta);
                        break;
                    case SplineTool.Rotate:
                        float angle = 0f;
                        if(e.mouseDelta.x > e.mouseDelta.y)
                        {
                            angle = e.mouseDelta.x;
                        }
                        else
                        {
                            angle = e.mouseDelta.y;
                        }
                        if(angle != 0f)
                        {
                            splineElement.RotateSpline(angle);
                            OnRotate?.Invoke(angle);
                        }
                        break;
                    case SplineTool.Scale:
                        break;
                }

                e.StopPropagation();

            }
            #endregion

            #region OnMouseUp
            protected virtual void OnMouseUp(MouseUpEvent e)
            {
                if (!m_Active || !target.HasMouseCapture() || !CanStopManipulation(e))
                    return;

                if(splineElement.Tool == SplineTool.Select) { splineElement.Selected = !splineElement.Selected; }

                m_Active = false;
                target.ReleaseMouse();
                e.StopPropagation();
            }
            #endregion
        }

        public class SplineManipulator : MouseManipulator

        {
            #region Init
            public SplineDrawingElement<T> splineElement;
            public VisualElement newHandle;
            
            public float halfHandle;
            protected bool m_Active;

            public SplineManipulator()
            {
                EventModifiers eventModifiers = EventModifiers.Control;
                activators.Add(new ManipulatorActivationFilter { button = MouseButton.LeftMouse, modifiers = eventModifiers  });
                m_Active = false;
            }
            #endregion

            #region Registrations
            protected override void RegisterCallbacksOnTarget()
            {

                target.RegisterCallback<MouseEnterEvent>(OnMouseEnter);
                target.RegisterCallback<MouseMoveEvent>(OnMouseMove);
                target.RegisterCallback<MouseUpEvent>(OnMouseUp);
                target.RegisterCallback<MouseLeaveEvent>(OnMouseLeave);
            }

            protected override void UnregisterCallbacksFromTarget()
            {
                target.UnregisterCallback<MouseEnterEvent>(OnMouseEnter);
                target.UnregisterCallback<MouseMoveEvent>(OnMouseMove);
                target.UnregisterCallback<MouseUpEvent>(OnMouseUp);
                target.UnregisterCallback<MouseLeaveEvent>(OnMouseLeave);
            }
            #endregion

            #region OnMouseEnter
            protected virtual void OnMouseEnter(MouseEnterEvent e)
            {
                if (m_Active)
                {
                    e.StopImmediatePropagation();
                    return;
                }


                m_Active = true;
                //target.CaptureMouse();
                e.StopPropagation();
                
            }
            #endregion

            #region OnMouseMove
            protected virtual void OnMouseMove(MouseMoveEvent e)
            {
                if (!m_Active ) return;

                if (e.ctrlKey)
                {
                    newHandle.style.display = DisplayStyle.Flex;
                    int closestAnchor = -1;
                    Vector2 closestPoint = splineElement.GetClosestPointOnSpline(e.localMousePosition, ref closestAnchor);
                    newHandle.style.left = closestPoint.x - halfHandle;
                    newHandle.style.top = closestPoint.y - halfHandle;
                }
                else
                {
                    newHandle.style.display = DisplayStyle.None;
                }
                e.StopPropagation();
            }
            #endregion

            #region OnMouseUp

            protected virtual void OnMouseUp(MouseUpEvent e)
            {
                if (!m_Active || !e.ctrlKey) { return; }
                Vector2 localPoint = e.localMousePosition;
                localPoint.y = (splineElement.worldBound.height - localPoint.y) / splineElement.worldBound.height;
                localPoint.x = localPoint.x / splineElement.worldBound.width;
                splineElement.AddPoint(localPoint);
            }

            #endregion
            protected virtual void OnMouseLeave(MouseLeaveEvent e)
            {
                
                if (!m_Active)
                    return;

                m_Active = false;
                newHandle.style.display = DisplayStyle.None;
                e.StopPropagation();
            }
        }

        public class SplineHandle : MouseManipulator
            
        {
            #region Init
            public SplineDrawingElement<T> splineElement;
            public event Action<Vector2> OnHandleUpdate;
            
            public float halfHandle;
            public bool dragX, dragY;
            private Vector2 m_Start;
            protected bool m_Active;

            public SplineHandle()
            {
                activators.Add(new ManipulatorActivationFilter { button = MouseButton.LeftMouse });
                m_Active = false;
            }
            #endregion

            #region Registrations
            protected override void RegisterCallbacksOnTarget()
            {
                target.RegisterCallback<MouseDownEvent>(OnMouseDown);
                target.RegisterCallback<MouseMoveEvent>(OnMouseMove);
                target.RegisterCallback<MouseUpEvent>(OnMouseUp);
            }

            protected override void UnregisterCallbacksFromTarget()
            {
                target.UnregisterCallback<MouseDownEvent>(OnMouseDown);
                target.UnregisterCallback<MouseMoveEvent>(OnMouseMove);
                target.UnregisterCallback<MouseUpEvent>(OnMouseUp);
            }
            #endregion

            #region OnMouseDown
            protected virtual void OnMouseDown(MouseDownEvent e)
            {
                if (m_Active)
                {
                    e.StopImmediatePropagation();
                    return;
                }

                if (CanStartManipulation(e))
                {
                    m_Start = e.localMousePosition;

                    m_Active = true;
                    target.CaptureMouse();
                    e.StopPropagation();
                }
            }
            #endregion

            #region OnMouseMove
            protected virtual void OnMouseMove(MouseMoveEvent e)
            {
                if (!m_Active || !target.HasMouseCapture()) return;
                float newLeft = target.style.left.value.value;
                float newTop = target.style.top.value.value;
                if (dragX)
                {
                    newLeft += e.mouseDelta.x;
                    newLeft = Mathf.Clamp(newLeft, target.parent.layout.xMin, target.parent.layout.xMax-(halfHandle));
                    target.style.left = newLeft;
                }

                if (dragY)
                {
                    newTop += e.mouseDelta.y;
                    newTop = Mathf.Clamp(newTop, target.parent.layout.yMin-halfHandle, target.parent.layout.yMax - halfHandle);
                    target.style.top = newTop;
                }
                e.StopPropagation();
                OnHandleUpdate?.Invoke(new Vector2(newLeft+halfHandle, newTop+halfHandle));
            }
            #endregion

            #region OnMouseUp
            protected virtual void OnMouseUp(MouseUpEvent e)
            {
                if (!m_Active || !target.HasMouseCapture() || !CanStopManipulation(e))
                    return;

                m_Active = false;
                target.ReleaseMouse();
                e.StopPropagation();
            }
            #endregion
        }
        #endregion
    }

}