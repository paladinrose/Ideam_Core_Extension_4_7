using System;
using System.Linq;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEditor;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Unity.UI;
using Ideam.Unity.Science.Formal.Mathematics;
using Ideam.Editor;

using ObjectField = UnityEditor.UIElements.ObjectField;

[CustomEditor(typeof(PathFollower))]
public class PathFollowerEditor : UIElementsEditor
{
    PathFollower pathFollower;
    Spline3HandleDrawer pathDrawer;

    VisualElement pathElement;
    DropList splinesList;
    int selectedPath = -1, drawingPath = -1;

    protected override void OnEnable()
    {
        pathFollower = (PathFollower)target;
    }

    protected override string[] ExcludedProperties()
    {
        return base.ExcludedProperties().Concat(new string[]
        {
            "boundPaths"
        }).ToArray();
    }


    public override VisualElement CreateInspectorGUI()
    {
        VisualElement insp = base.CreateInspectorGUI();
        
        insp.AddToClassList("background");
        splinesList = new DropList("Paths");
        int bp = pathFollower.boundPaths.Length;
        string[] pathLabels = new string[bp];

        for (int i = 0; i < bp; i++)
        {
            pathLabels[i] = "Path " + (i + 1);

        }

        splinesList.BuildList(pathLabels);

        splinesList.OnAdd += AddPath;
        splinesList.OnSelect += ShowPathInInspector;

        insp.Add(splinesList);

        pathElement = new VisualElement();
        pathElement.AddToClassList("background");
        pathElement.style.minHeight = 60;
        insp.Add(pathElement);

        return insp;
    }

    private void ShowPathInInspector(int pathID)
    {
        selectedPath = pathID;
        
        pathElement.Clear();

        if(pathID < 0) { return; } 

        ObjectField pathTargetField = new UnityEditor.UIElements.ObjectField("Follow Target");
        pathTargetField.objectType = typeof(Transform);
        pathTargetField.value = pathFollower.boundPaths[pathID].boundToPath;
        pathTargetField.RegisterValueChangedCallback(evt => { pathFollower.boundPaths[pathID].boundToPath = (Transform)evt.newValue; });
        pathElement.Add(pathTargetField);

        ObjectField pathSpaceField = new UnityEditor.UIElements.ObjectField("Path Local Space");
        pathSpaceField.objectType = typeof(Transform);
        pathSpaceField.value = pathFollower.boundPaths[pathID].pathSpace;
        pathSpaceField.RegisterValueChangedCallback(evt => { pathFollower.boundPaths[pathID].pathSpace = (Transform)evt.newValue; });
        pathElement.Add(pathSpaceField);

    }

    private void AddPath(int newPathID)
    {
        Array.Resize(ref pathFollower.boundPaths, newPathID+1);
        
        BoundPath newBoundPath = new BoundPath()
        {
            path = new Ideam.Science.Formal.Mathematics.Geometry.Splines.Spline3(3, Ideam.Science.Formal.Mathematics.Geometry.Splines.CurveType.CatmullRom)
        };

        pathFollower.boundPaths[newPathID] = newBoundPath;
        
        splinesList.listElements[newPathID].Label = "Path " + (newPathID + 1);
    }

    private void OnSceneGUI()
    {
        if (!pathFollower || selectedPath <0) return;

        if(selectedPath != drawingPath)
        {
            drawingPath = selectedPath;
            pathDrawer = new Spline3HandleDrawer(new Spline3[] { pathFollower.boundPaths[selectedPath].path }, false,
                                                 pathFollower.boundPaths[selectedPath].pathSpace);
            pathDrawer.OnStopPointMove += () => 
            {
                serializedObject.ApplyModifiedProperties();
                EditorUtility.SetDirty(pathFollower);
            };
        }
        pathDrawer.DrawSplineAndHandles();
    }
}

