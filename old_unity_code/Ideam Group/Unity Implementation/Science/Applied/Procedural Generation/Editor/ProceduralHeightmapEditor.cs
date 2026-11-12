using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEditor;
using UnityEngine.UIElements;

using ProceduralToolkit.FastNoiseLib;

using Ideam.Unity.Science.Formal.Mathematics;
using Ideam.Unity.UI;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Editor;


[CustomEditor(typeof(ProceduralHeightmap))]
public class ProceduralHeightmapEditor : UIElementsEditor
{
    protected override string[] ExcludedProperties()
    {
        return base.ExcludedProperties().Concat(new string[]
            {
               "regions", "heightmapSize"
            }).ToArray();
    }


    protected ProceduralHeightmap phm;
    protected float lineSize = 1f;
    VisualElement windowRoot, windowElement, generatedMap, selectedRegion, selectedFilter;
    Vector2IntField sizeField;
    ScrollView windowScroll;
    Spline2DrawingElement[] regionDrawers;
    RegionWindow window;
    protected override void OnEnable()
    {
        base.OnEnable();
        phm = (ProceduralHeightmap) target;
        phm.UpdateHeightmap();
    }

    protected override void OnDisable()
    {
        base.OnDisable();
    }

    public override VisualElement CreateInspectorGUI()
    {
        base.CreateInspectorGUI();
        sizeField = new Vector2IntField("Heightmap Size") { value = phm.heightmapSize};
        sizeField.RegisterValueChangedCallback(evt => { phm.heightmapSize = evt.newValue; });
        root.Add(sizeField);
        Button openWindow = new Button(OpenRegionWindow);
        openWindow.text = "Edit Heightmap";
        root.Add(openWindow);

        UpdateHeightmap(-1);
        return root;
    }

    private void OpenRegionWindow()
    {
        window = RegionWindow.OpenWindow();
        windowRoot = window.rootVisualElement;
        windowRoot.styleSheets.Add(ideOS);
        windowRoot.AddToClassList("ideamRoot");
        windowRoot.style.flexDirection = FlexDirection.Row;

        windowScroll = new ScrollView(ScrollViewMode.VerticalAndHorizontal);
        windowRoot.Add(windowScroll);
        

        windowElement = new VisualElement();
        windowElement.style.width = phm.heightmapSize.x+20;
        windowElement.style.height = phm.heightmapSize.y+20;
        windowScroll.Add(windowElement);

        generatedMap = new VisualElement();
        generatedMap.style.position = Position.Absolute;
        generatedMap.style.left = 10f; generatedMap.style.right = 10f;
        generatedMap.style.top = 10f; generatedMap.style.bottom = 10f;
        generatedMap.style.backgroundImage = phm.Heightmap;
        windowElement.Add(generatedMap);

        regionDrawers = new Spline2DrawingElement[phm.regions.Length];

        for(int i = 0; i < phm.regions.Length; i++)
        {
            int id = i;
            Spline2DrawingElement regiondrawer = new Spline2DrawingElement(phm.regions[i].regionBounds);
            regiondrawer.RenderScale = 1f;
            
            generatedMap.Add(regiondrawer);
            regionDrawers[i] = regiondrawer;
        }

        VisualElement windowToolbar = new VisualElement();
        windowToolbar.style.minWidth = 200;
        windowRoot.Add(windowToolbar);

        Button newRegionButton = new Button(NewRegion);
        newRegionButton.text = "New Region";
        windowToolbar.Add(newRegionButton);
        
        DropList regionsDrop = new DropList("Regions");
        regionsDrop.ShowSearch = false;
        string[] regionNames = new string[phm.regions.Length];
        for(int i =0; i < regionNames.Length; i++)
        {
            regionNames[i] = phm.regions[i].name;
        }
        regionsDrop.BuildList(regionNames);
        regionsDrop.OnSelect += SelectRegion;
        regionsDrop.OnRemove += RemoveRegion;
        windowToolbar.Add(regionsDrop);
        selectedRegion = new VisualElement();

        windowToolbar.Add(selectedRegion);

        
    }

    private void RemoveRegion(int obj)
    {
        List<ProceduralHeightmap.MapRegion> newRegions = new List<ProceduralHeightmap.MapRegion>(phm.regions);
        newRegions.RemoveAt(obj);
        phm.regions = newRegions.ToArray();
        UpdateObject();
    }

    private void SelectRegion(int id)
    {
        for(int i =0; i < phm.regions.Length; i++)
        {
            if(i == id) { regionDrawers[i].Selected = true; continue; }

            regionDrawers[i].Selected = false;
        }
        selectedRegion.Clear();
        
        if(id < 0) { return; }
        
        ProceduralHeightmap.MapRegion mapRegion = phm.regions[id];

        TextField regionNameField = new TextField("Region Name") { value = mapRegion.name, isDelayed = true };
        regionNameField.RegisterValueChangedCallback(evt => { phm.regions[id].name = evt.newValue; });
        selectedRegion.Add(regionNameField);

        EnumField noiseTypeField = new EnumField("Noise Type", mapRegion.noiseType);
        noiseTypeField.RegisterValueChangedCallback(evt => { phm.regions[id].noiseType = (FastNoise.NoiseType)evt.newValue; UpdateHeightmap(id); });
        selectedRegion.Add(noiseTypeField);

        IntegerField seedField = new IntegerField("Seed") { value = mapRegion.seed, isDelayed = true };
        seedField.RegisterValueChangedCallback(evt => { phm.regions[id].seed = evt.newValue; UpdateHeightmap(id); });
        selectedRegion.Add(seedField); UpdateHeightmap(id);
        EnumField noiseInterpField = new EnumField("Noise Interpolation", mapRegion.noiseInterpolation);
        noiseInterpField.RegisterValueChangedCallback(evt => { phm.regions[id].noiseInterpolation = (FastNoise.Interp)evt.newValue; UpdateHeightmap(id); });
        selectedRegion.Add(noiseInterpField);

        EnumField noiseFractalField = new EnumField("Noise Fractal", mapRegion.noiseFractal);
        noiseFractalField.RegisterValueChangedCallback(evt => { phm.regions[id].noiseFractal = (FastNoise.FractalType)evt.newValue; UpdateHeightmap(id); });
        selectedRegion.Add(noiseFractalField);

        DropList filtersDrop = new DropList("Filters");
        

        selectedFilter = new VisualElement();
        selectedRegion.Add(selectedFilter);
        /*
        public MapRegionFilter[] filters = new MapRegionFilter[0];
        */
    }

    void SelectMapRegionFilter(int id)
    {
       
    }

    private void NewRegion()
    {
        int id = phm.NewRegion();

        Spline2DrawingElement regiondrawer = new Spline2DrawingElement(phm.regions[id].regionBounds);
        regiondrawer.RenderScale = 1f;
        //I want to expand this into a handle that changes function based on the selected Unity tool, or my own small toolbar.
        //VisualElement moveRegionControl = new VisualElement();
        //moveRegionControl.RegisterCallback<MouseDownEvent>(evt => { StartDraggingRegion(id, evt); });
        //moveRegionControl.RegisterCallback<MouseUpEvent>(evt => { StopDraggingRegion(id, evt); });
        //regiondrawer.Add(moveRegionControl);

        windowElement.Insert(windowElement.childCount-2,regiondrawer);
        
    }
    protected virtual void UpdateHeightmap(int id)
    {
        if (id < 0)
        {
            for (int i = 0; i < phm.regions.Length; i++)
            {
                phm.regions[i].noise = null;
            }
        }
        else
        {
            phm.regions[id].noise = null;
        }

        phm.UpdateHeightmap();

        if (window != null)
        {
            windowElement.style.width = phm.heightmapSize.x + 20;
            windowElement.style.height = phm.heightmapSize.y + 20;

            generatedMap.style.backgroundImage = phm.Heightmap;
        }

        UpdateObject();
    }

    class RegionWindow : EditorWindow
    {
        public static RegionWindow OpenWindow()
        {
            RegionWindow window = (RegionWindow)EditorWindow.GetWindow(typeof(RegionWindow));
            window.Show();

            return window;
        }

    }
}

