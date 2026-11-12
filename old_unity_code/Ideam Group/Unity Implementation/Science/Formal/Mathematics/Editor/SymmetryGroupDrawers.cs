using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEditor;

using Ideam.Science.Formal.Graphs;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling;

using Ideam.Unity.UI;
using Ideam.Unity.UI.AnytimeGraphs;


namespace Ideam.Unity
{
    public static class SymmetryGroupDrawers
    {
    }

    public abstract class SymmetryElement<T> : VisualElement
        where T : ISymmetry
    {
        /*
         int Generation { get; set; }
        Float2 Position { get; set; }
        Float2 Scale { get; set; }
        float Angle { get; set; }
        bool IsOnBorder { get; set; }
        int[] Vertices { get; set; }
        int[] Edges { get; set; }
        int[] Triangles { get; }
        int[] Neighbors { get; set; }
         */
        protected Foldout symmetryStats;
        protected IntegerField generationLabel;
        protected FloatField angleLabel;
        protected Toggle isOnBorderLabel;
        protected Vector2Field positionLabel, scaleLabel;

        public SymmetryElement(T sym)
        {
            symmetryStats = new Foldout() { text = "Symmetry Info", value = false };
            Add(symmetryStats);
            
            generationLabel = new IntegerField("Generation");
            generationLabel.value = sym.Generation;
            generationLabel.SetEnabled(false);
            symmetryStats.Add(generationLabel);

            positionLabel = new Vector2Field("Position");
            positionLabel.value = sym.Position.ToVector2();
            positionLabel.SetEnabled(false);
            symmetryStats.Add(positionLabel);

            angleLabel = new FloatField("Angle");
            angleLabel.value = sym.Angle;
            angleLabel.SetEnabled(false);
            symmetryStats.Add(angleLabel);

            scaleLabel = new Vector2Field("Scale");
            scaleLabel.value = sym.Scale.ToVector2();
            scaleLabel.SetEnabled(false);
            symmetryStats.Add(scaleLabel);

            isOnBorderLabel = new Toggle("Is On Border") { value = sym.IsOnBorder };
            isOnBorderLabel.SetEnabled(false);
            symmetryStats.Add(isOnBorderLabel);
        }
    }

    public class IsoSymmetryElement : SymmetryElement<IsoSymmetry>
    {
        public IsoSymmetryElement(IsoSymmetry sym) : base(sym)
        {

        }
    }

    public class SymmetryEdgeElement : VisualElement 
    {
        public SymmetryEdgeElement(SymmetryEdge edge)
        {

        }
    }

    public abstract class SymmetryGroupElement<T, TSymmetry, TSymmetryElement> : AnytimeGraphView
        where T:SymmetryGroup<T, TSymmetry> 
        where TSymmetry : ISymmetry
        where TSymmetryElement :SymmetryElement<TSymmetry>
    {

        protected TSymmetryElement symmetryTemplateField;
        protected EnumField buildPhaseField;
        protected Foldout symmetriesFoldout, vertexFoldout, edgeFoldout;
        protected TSymmetryElement[] symmetryFields;

        protected SymmetryEdgeElement[] edgeFields;

        public SymmetryGroupElement(T group) : base()
        {
            LinkToGraph(group);
        }
    }

    public class ComplexSymmetryGroupElement :SymmetryGroupElement<ComplexSymmetryGroup, IsoSymmetry, IsoSymmetryElement>
    {
        public ComplexSymmetryGroupElement(ComplexSymmetryGroup group) : base(group)
        {

        }
    }

    public abstract class SymmetryGroupHandlesDrawer<T, TSymmetry, TElement, TSymmetryElement>
        where T : SymmetryGroup<T, TSymmetry>
        where TSymmetry : ISymmetry
        where TElement:SymmetryGroupElement<T, TSymmetry, TSymmetryElement>
        where TSymmetryElement : SymmetryElement<TSymmetry>
    {
        public virtual event Action<int> OnSelectSymmetryGroup, OnSelectSymmetry;
        protected Transform localizer;

        protected int currentPointID = -1, currentSymmetryID = -1, currentEdgeID = -1, currentSymmetryGroupID = -1;
        protected int[] currentGroupSelection, currentPointSelection, currentSymmetrySelection, currentEdgeSelection;

        protected T[] groups, worldGroups;
        protected TElement[] groupElements;

        protected Vector3[] storedPoints, currentPoints;
        public Color selectedSymmetryColor = Color.white, 
                     unselectedSymmetryColor = Color.red;
        public virtual TElement[] GroupElements { get { return groupElements; } }

        public SymmetryGroupHandlesDrawer(T[] symGroups, Transform transform = null, VisualElement groupElementsParent = null)
        {
            localizer = transform;
            groups = symGroups;
        }

        public virtual void DrawSymmetryGroupAndHandles(bool[] handleSelection)
        {
            if (currentSymmetryGroupID < 0 || handleSelection.Length != groups[currentSymmetryGroupID].NodesCount) { return; }
            
            DrawSymmetryGroup();

            DrawHandles(handleSelection);
        }

        public abstract void DrawSymmetryGroup();
        public abstract void DrawHandles(bool[] handleSelection);

        public virtual void SelectSymmetryGroup(int id)
        {
            if (id >= groups.Length || id < 0) { id = -1; }
            if (currentSymmetryGroupID >= 0) { groupElements[currentSymmetryGroupID].style.display = DisplayStyle.None; }
            currentSymmetryGroupID = id;
            if (currentSymmetryGroupID >= 0) { groupElements[currentSymmetryGroupID].style.display = DisplayStyle.Flex; }
            OnSelectSymmetryGroup?.Invoke(currentSymmetryGroupID);
        }

       
    }

    public class ComplexSymmetryGroupHandlesDrawer : SymmetryGroupHandlesDrawer<ComplexSymmetryGroup, IsoSymmetry, ComplexSymmetryGroupElement, IsoSymmetryElement>
    {
        public override event Action<int> OnSelectSymmetry;
        public ComplexSymmetryGroupHandlesDrawer(ComplexSymmetryGroup[] symGroups, Transform transform = null, VisualElement groupElementsParent = null) : base(symGroups, transform, groupElementsParent)
        {
            if (groupElementsParent != null)
            {
                groupElements = new ComplexSymmetryGroupElement[groups.Length];
                for(int i =0; i < groups.Length; i++)
                {
                    groupElements[i] = new ComplexSymmetryGroupElement(groups[i]);
                    
                    groupElementsParent.Add(groupElements[i]);
                    groupElements[i].style.display = DisplayStyle.None;
                }
            }
            
            if(symGroups.Length > 0) { SelectSymmetryGroup(0); }
        }

        public override void SelectSymmetryGroup(int id)
        {
            base.SelectSymmetryGroup(id);
            if(currentSymmetryGroupID < 0) { return; }

            ComplexSymmetryGroup group = groups[currentSymmetryGroupID];

            storedPoints = group.Vertices.ToVector3(group.axisConversion);
            if (localizer)
            {
                storedPoints = localizer.TransformPoints(storedPoints);
            }
        }
        public override void DrawHandles(bool[] handleSelection)
        {
            ComplexSymmetryGroup group = groups[currentSymmetryGroupID];

            var syms = group.symmetries;
            for (int i = 0; i < syms.Length; i++)
            {
                int id = i;
                Vector3 symPos = Vector3.zero;

                if (localizer) { symPos = localizer.TransformPoint(syms[i].Position.ToVector3(group.axisConversion)); }
                else { symPos = syms[i].Position.ToVector3(group.axisConversion); }

                if (handleSelection[i])
                {
                    Handles.color = selectedSymmetryColor;
                }
                else
                {
                    Handles.color = unselectedSymmetryColor;
                }

                float hs = HandleUtility.GetHandleSize(symPos) * 0.05f;
               if( Handles.Button(symPos, UnityEngine.Quaternion.identity, hs, hs * 1.2f, Handles.CubeHandleCap))
                {
                    OnSelectSymmetry?.Invoke(id);
                }
            }
        }
        
        static Color[] colorWheel = new Color[]
        {
            Color.red, Color.yellow, Color.green, Color.cyan, Color.blue, Color.magenta
        };

        public override void DrawSymmetryGroup()
        {
            if (currentSymmetryGroupID < 0) { return; }
            ComplexSymmetryGroup group = groups[currentSymmetryGroupID];
            var edges = group.edges;
            
            var syms = group.symmetries;

            for(int i =0; i < syms.Length; i++)
            {
                Vector3 symPos = Vector3.zero;
                
                if (localizer) { symPos = localizer.TransformPoint(syms[i].Position.ToVector3(group.axisConversion)); }
                else { symPos = syms[i].Position.ToVector3(group.axisConversion); }

                int[] vertIds = syms[i].vertices;
                int l = vertIds.Length-1;
                
                Vector3 p1 = Vector3.zero, p2 = Vector3.zero;

                for(int v = 0; v < l; v++)
                {
                    p1 = storedPoints[vertIds[v]];
                    p2 = storedPoints[vertIds[v + 1]];
                    p1 = Vector3.Lerp(symPos, p1, 0.975f);
                    p2 = Vector3.Lerp(symPos, p2, 0.975f);
                    Handles.color = colorWheel[v];
                    Handles.DrawLine(p1, p2);
                }
                Handles.color = colorWheel[l];
                p1 = storedPoints[vertIds[l]];
                p1 = Vector3.Lerp(symPos, p1, 0.975f);
                p2 = storedPoints[vertIds[0]];
                p2 = Vector3.Lerp(symPos, p2, 0.975f);
                Handles.DrawLine(p1, p2);
            }

            for (int i = 0; i < group.EdgesCount; i++)
            {
                var edge = edges[i];
                Vector3 p1 = storedPoints[edge.PortA], p2 = storedPoints[edge.PortB];
                if (edge.isBorder)
                {
                    Handles.color = Color.white;

                }
                else { Handles.color = Color.black; }
                Handles.DrawLine(p1, p2);
            }

        }
    }
}