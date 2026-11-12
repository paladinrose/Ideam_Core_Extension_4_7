using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEditor;

using Ideam.Unity.UI;
using Ideam.Unity.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Editor;


using Phase = Ideam.Unity.Science.Applied.ProceduralGeneration.ProceduralWallpaper.WallpaperPhase;

using ProgressBar = Ideam.Unity.UI.ProgressBar;

namespace Ideam.Unity.Science.Applied.ProceduralGeneration
{
    [CustomEditor(typeof(ProceduralWallpaper))]
    public class ProceduralWallpaperEditor : UIElementsEditor
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[] 
            {
               "wallpaper", "wallpaperType", "wallpaperInstances", "phase", "regionsToWallpaper", "keepInstance"
            }).ToArray();
        }

        protected ProceduralWallpaper pw;
        
        protected Spline2HandleDrawer regionDrawer = null;
        protected ComplexSymmetryGroupHandlesDrawer wallpaperDrawer = null;

        protected EnumField phaseField, wallpaperTypeField;
        protected Button wallpaperButton, populateButton;
        protected ProgressBar wallpaperProgress;
        protected DropList regionsDrop;
        protected Toggle showKeepInstanceToggles;
        protected VisualElement selectedRegionElement, selectedGroupElement;
        
        protected int selectedRegionID;
        protected IEnumerator paperProcess;
        protected Phase currentPhase;

        protected override void OnEnable()
        {
            base.OnEnable();
            pw = (ProceduralWallpaper)target;
            
        }

        protected override void OnDisable()
        {
            base.OnDisable();
            if(regionDrawer != null)regionDrawer.OnStopPointMove -= UpdateRegions;
            
        }

        public override VisualElement CreateInspectorGUI()
        {
            base.CreateInspectorGUI();
            currentPhase = pw.Phase;

            phaseField = new EnumField("Phase", currentPhase);
            phaseField.RegisterValueChangedCallback(SetPhase);
            root.Add(phaseField);

            regionsDrop = new DropList("Regions To Wallpaper");
            string[] regionNames = new string[pw.regionsToWallpaper.Length];
            for(int i = 0; i < regionNames.Length; i++)
            {
                regionNames[i] = pw.regionsToWallpaper[i].name;
            }
            regionsDrop.BuildList(regionNames);
            regionsDrop.Select(0);
            regionsDrop.OnAdd += AddRegion;
            regionsDrop.OnRemove += RemoveRegion;
            regionsDrop.OnSelect += SelectRegion;
            root.Add(regionsDrop);

            wallpaperTypeField = new EnumField("Wallpaper Type", pw.wallpaperType);
            root.Add(wallpaperTypeField);
            wallpaperTypeField.RegisterValueChangedCallback(SetWallpaperType);
            if(currentPhase == Phase.Uninitialized) { wallpaperTypeField.style.display = DisplayStyle.None;  }

            wallpaperButton = new Button(WallpaperRegions);
            wallpaperButton.text = "Wallpaper Regions";
            root.Add(wallpaperButton);
            if (currentPhase == Phase.Uninitialized || pw.wallpaperType == SymmetryGroupType.None) { wallpaperButton.style.display = DisplayStyle.None; }

            wallpaperProgress = new ProgressBar();
            root.Add(wallpaperProgress);
            wallpaperProgress.style.display = DisplayStyle.None;

            VisualElement spacer = new VisualElement();
            root.Add(spacer);
            spacer.style.height = 20;

            selectedRegionElement = new VisualElement();
            root.Add(selectedRegionElement);

            selectedGroupElement = new VisualElement();
            root.Add(selectedGroupElement);

            showKeepInstanceToggles = new Toggle("Show Keep Instance Toggles");
            root.Add(showKeepInstanceToggles);

            populateButton = new Button(PopulateWallpaper) { text = "Populate Wallpaper" };
            root.Add(populateButton);


            SetNewPhase(currentPhase);

            return root;
        }
        private void PopulateWallpaper()
        {
            if (pw.Phase != Phase.AssetDistribution) return;
            paperProcess = pw.PopulateWallpaper();
            EditorApplication.update += PopulateProcess;
        }

        private void PopulateProcess()
        {
            if (!paperProcess.MoveNext())
            {
                paperProcess = null;
                EditorApplication.update -= PopulateProcess;
                Debug.Log("Populating Complete!");
            }
        }

        private void SetWallpaperType(ChangeEvent<Enum> obj)
        {
            SymmetryGroupType newType = (SymmetryGroupType)obj.newValue;

            if(newType != SymmetryGroupType.None)
            {
                wallpaperButton.style.display = DisplayStyle.Flex;
            }

            pw.wallpaperType = newType;
            UpdateTarget();
        }

        protected override void OnSceneGUI(SceneView sceneView)
        {
            switch (currentPhase)
            {
                case Phase.Uninitialized:
                    break;

                case Phase.RegionSetup:
                    if (regionDrawer != null) regionDrawer.DrawSplineAndHandles();
                    break;

                case Phase.WallpaperSetup:
                    if (regionDrawer != null) regionDrawer.DrawSplineAndHandles();
                    if (wallpaperDrawer != null) wallpaperDrawer.DrawSymmetryGroup();
                    break;

                case Phase.AssetDistribution:
                    if (showKeepInstanceToggles.value)
                    {
                        wallpaperDrawer.DrawSymmetryGroupAndHandles(pw.keepInstance);
                    }
                    break;

                case Phase.Complete:
                    break;
            }
        }

        #region Regions to Wallpaper

        private void SelectRegion(int obj)
        {
            regionDrawer.SelectSpline(obj);
        }

        private void AddRegion(int obj)
        {
            string newRegionName = regionsDrop.listElements[obj].text;

            List<Spline2> newRegions = new List<Spline2>(pw.regionsToWallpaper);

            var newRegion = new Spline2(3);
            newRegion.axisConversion = XYToXYZ.XZ;
            newRegion.Wrap = true;
            newRegion.name = newRegionName;

            newRegions.Add(newRegion);

            pw.regionsToWallpaper = newRegions.ToArray();
            
            selectedRegionElement.Clear();

            if(regionDrawer != null)regionDrawer.OnStopPointMove -= UpdateRegions;
            regionDrawer = new Spline2HandleDrawer(pw.regionsToWallpaper, false, pw.transform, selectedRegionElement);
            regionDrawer.OnStopPointMove += UpdateRegions;

            UpdateTarget();
            regionsDrop.Select(obj);
        }
        
        private void RemoveRegion(int obj)
        {
            List<Spline2> newRegions = new List<Spline2>(pw.regionsToWallpaper);
            newRegions.RemoveAt(obj);
            pw.regionsToWallpaper = newRegions.ToArray();

            selectedRegionElement.Clear();

            if (regionDrawer != null) regionDrawer.OnStopPointMove -= UpdateRegions;
            if (pw.regionsToWallpaper.Length > 0)
            {
                regionDrawer = new Spline2HandleDrawer(pw.regionsToWallpaper, false, pw.transform, selectedRegionElement);
                regionDrawer.OnStopPointMove += UpdateRegions;
            }
            UpdateTarget();
        }

        #endregion

        #region Wallpapering Process

        float regionVolume, symmetryVolume = -1;

        private void WallpaperRegions()
        {
            wallpaperButton.SetEnabled(false);
            wallpaperProgress.style.display = DisplayStyle.Flex;

            regionVolume = pw.regionsToWallpaper.Area();

            paperProcess = ComplexSymmetryGroup.WallpaperProcess(pw.wallpaperType, pw.regionsToWallpaper, pw.regionsToWallpaper.GetExtents().Center, WallpaperComplete);
            EditorApplication.update += PaperProcess;
        }

        private void UpdateWallpaperProgressBar(int obj)
        {
            if(obj < 1) { return; }
            if(obj == 1)
            {
                Float2[] symPoints = pw.wallpaper.PrimitiveVertices;

                Spline2 symSpline = new Spline2(symPoints);

                symmetryVolume = symSpline.Area();
            }
            Debug.Log("Build: " + obj);
            wallpaperProgress.value = symmetryVolume * obj / regionVolume;
        }


        void PaperProcess()
        {
            
            if (!paperProcess.MoveNext())
            {
                EditorApplication.update -= PaperProcess;
                paperProcess = null;
                wallpaperProgress.style.display = DisplayStyle.None;
            }
            else
            {
               // wallpaperProgress.value = symmetryVolume *? / regionVolume;
            }
        }

        void WallpaperComplete(ComplexSymmetryGroup completedWallpaper)
        {
            pw.wallpaper = completedWallpaper;
            pw.wallpaper.axisConversion = XYToXYZ.XZ;

            pw.ValidateInstances();

            phaseField.value = Phase.WallpaperSetup;
            Debug.Log("Wallpapering Complete!");
        }
        #endregion

        private void SetPhase(ChangeEvent<Enum> obj)
        {
            var oldPhase = pw.Phase;
            var newPhase = (Phase)obj.newValue;

            if(oldPhase == newPhase) { return; }

            if (!ResolveOldPhase(oldPhase)) { return; }
            if (!SetNewPhase(newPhase)) { SetNewPhase(oldPhase); }

            currentPhase = pw.Phase = newPhase;

            UpdateTarget();
        }

        bool ResolveOldPhase(Phase oldPhase)
        {
            switch (oldPhase)
            {
                case Phase.Uninitialized:

                    if (pw.regionsToWallpaper.Length == 0)
                    {
                        return false;
                    }

                    break;

                case Phase.RegionSetup:

                    if (regionDrawer != null) 
                    {
                        //selectedRegionElement.Clear();
                        //regionDrawer.OnStopPointMove -= UpdateRegions; regionDrawer = null;
                    }

                    break;

                case Phase.WallpaperSetup:

                    if(wallpaperDrawer != null)
                    {
                        //selectedGroupElement.Clear();
                        //wallpaperDrawer = null;
                    }

                    break;

                case Phase.AssetDistribution:

                    break;

                case Phase.Complete:

                    
                    break;
            }
            return true;
        }

        bool SetNewPhase(Phase newPhase)
        {
            switch (newPhase)
            {
                case Phase.Uninitialized:
                    
                    break;

                case Phase.RegionSetup:

                    selectedRegionElement.Clear();
                    
                    regionDrawer = new Spline2HandleDrawer(pw.regionsToWallpaper, false, pw.transform, selectedRegionElement);
                    regionDrawer.OnStopPointMove += UpdateRegions;
                    regionDrawer.SelectSpline(0);
                    wallpaperTypeField.style.display = DisplayStyle.Flex;

                    break;

                case Phase.WallpaperSetup:

                    selectedRegionElement.Clear();

                    regionDrawer = new Spline2HandleDrawer(pw.regionsToWallpaper, false, pw.transform, selectedRegionElement);
                    regionDrawer.OnStopPointMove += UpdateRegions;
                    regionDrawer.SelectSpline(0);
                   



                    selectedGroupElement.Clear();

                    wallpaperDrawer = new ComplexSymmetryGroupHandlesDrawer(new ComplexSymmetryGroup[] { pw.wallpaper }, pw.transform, selectedGroupElement);
                    wallpaperDrawer.SelectSymmetryGroup(0);

                    break;

                case Phase.AssetDistribution:

                    
                    selectedGroupElement.Clear();

                    wallpaperDrawer = new ComplexSymmetryGroupHandlesDrawer(new ComplexSymmetryGroup[] { pw.wallpaper }, pw.transform, selectedGroupElement);
                    wallpaperDrawer.SelectSymmetryGroup(0);
                    wallpaperDrawer.OnSelectSymmetry += evt => { pw.keepInstance[evt] = !pw.keepInstance[evt]; };

                    break;

                case Phase.Complete:
                    break;
            }
            return true;
        }

        

        void UpdateRegions()
        {
            UpdateTarget();   
        }

        void UpdateTarget()
        { 
            EditorUtility.SetDirty(pw);
            PrefabUtility.RecordPrefabInstancePropertyModifications(pw);
            serializedObject.ApplyModifiedProperties();
        }
    }
}