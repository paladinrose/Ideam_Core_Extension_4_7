using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;
using Ideam.Unity.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    [UserInterface(typeof(AnimatorControl))]
    public class AnimatorControlUI : BaseControlUI<AnimatorControl>
    {
        public DropList blendShapeDataList;
        public VisualElement selectedBlendShapeElement;
        public BlendShapeDataDrawer blendShapeDataDrawer;

        public override void SubscribeToObjectEvents()
        {
        }

        public override void UnsubscribeFromObjectEvents()
        {
        }


        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }

            blendShapeDataList = ui.Q<DropList>("BlendShapeDataList");
            selectedBlendShapeElement = ui.Q<VisualElement>("SelectedBlendShape");

            blendShapeDataList.OnAdd += AddNewBlendShapeData;
            blendShapeDataList.OnRemove += RemoveBlendShapeData;
            blendShapeDataList.OnSelect += evt =>
            {
                if (blendShapeDataDrawer == null) { blendShapeDataDrawer = new BlendShapeDataDrawer(); }
                selectedBlendShapeElement.Clear();
                if (evt < 0) return;
                selectedBlendShapeElement.Add(blendShapeDataDrawer.DrawBlendShapeData(sourceObject.blendShapeData[evt]));
            };

            UpdateBlendShapeDataList();

            return true;
        }

        private void UpdateBlendShapeDataList()
        {
            string[] bnames = new string[sourceObject.blendShapeData.Length];
            for (int i = 0; i < bnames.Length; i++)
            {
                bnames[i] = sourceObject.blendShapeData[i].name;
            }
            blendShapeDataList.BuildList(bnames);
            
        }

        private void AddNewBlendShapeData(int obj)
        {
            string newBlendShapeDataName = blendShapeDataList.listElements[obj].Label;

            List<AnimatorControl.BlendShapeData> newData = new List<AnimatorControl.BlendShapeData>(sourceObject.blendShapeData);
            AnimatorControl.BlendShapeData nbsd = new AnimatorControl.BlendShapeData() 
            {
                name = newBlendShapeDataName, 
                blending = new BlendShapesGesture(null),
                controlAnimatorID = -1
            };
            newData.Add(nbsd);
            sourceObject.blendShapeData = newData.ToArray();
            UpdateBlendShapeDataList();
        }

        private void RemoveBlendShapeData(int obj)
        {
            List<AnimatorControl.BlendShapeData> newData = new List<AnimatorControl.BlendShapeData>(sourceObject.blendShapeData);
            newData.RemoveAt(obj);
            sourceObject.blendShapeData = newData.ToArray();
            UpdateBlendShapeDataList();
        }
    }


    public class BlendShapeDataDrawer
    {
        AnimatorControl.BlendShapeData blendShapeData;
        public ObjectSelectorField skinSelector;
        public Slider currentBlendSlider;
        public Foldout blendShapesFoldout;
        public DropList addBlendShapesList;
        public ScrollView blendShapeSplines;
        public Spline1DrawingElement[] blendShapeDrawingElements;

        string[] blendShapesOnSkin = new string[0];
        bool[] addBlendShapesMask = new bool[0];

        public VisualElement DrawBlendShapeData(AnimatorControl.BlendShapeData blendShapeData)
        {
            this.blendShapeData = blendShapeData;
            
            VisualElement blendShapeDataElement = new VisualElement();

            skinSelector = new ObjectSelectorField("Skinned Mesh");
            skinSelector.Initialize<SkinnedMeshRenderer>(SetSkin, blendShapeData.blending.Skin);
            blendShapeDataElement.Add(skinSelector);

            currentBlendSlider = new Slider(0, 1, SliderDirection.Horizontal) { label = "Current Blend %", value = blendShapeData.CurrentBlendPercent };
            currentBlendSlider.RegisterValueChangedCallback(evt => { blendShapeData.SetCurrentBlend(evt.newValue); blendShapeData.UpdateCurrentBlend(); });
            blendShapeDataElement.Add(currentBlendSlider);

            addBlendShapesList = new DropList("Add BlendShapes");
            addBlendShapesList.MaskEntries(addBlendShapesMask);
            addBlendShapesList.OnSelect += AddBlendShapeToData;
            blendShapeDataElement.Add(addBlendShapesList);
            UpdateAddBlendShapesList();

            blendShapesFoldout = new Foldout() { text = "Blend Shape Splines", value = false };

            blendShapeDataElement.Add(blendShapesFoldout);

            blendShapeSplines = new ScrollView(ScrollViewMode.Vertical);
            blendShapesFoldout.Add(blendShapeSplines);

            UpdateBlendShapeSplines();
            return blendShapeDataElement;
        }

        private void SetSkin(SkinnedMeshRenderer selection)
        {
            blendShapeData.blending.SetSkin(selection);
            UpdateAddBlendShapesList();
        }

        private void UpdateAddBlendShapesList()
        {
            if(blendShapeData.blending.Skin == null)
            {
                blendShapesOnSkin = new string[0];
                addBlendShapesMask = new bool[0];

                addBlendShapesList.BuildList(blendShapesOnSkin);
                return;
            }

            Mesh skinMesh = blendShapeData.blending.Skin.sharedMesh;

            blendShapesOnSkin = new string[skinMesh.blendShapeCount];
            addBlendShapesMask = new bool[skinMesh.blendShapeCount];

            for (int i = 0; i < skinMesh.blendShapeCount; i++)
            {
                blendShapesOnSkin[i] = skinMesh.GetBlendShapeName(i);
                addBlendShapesMask[i] = true;
                for (int j = 0; j < blendShapeData.blending.blendShapeIDs.Length; j++)
                {
                    if (blendShapeData.blending.blendShapeIDs[j] == i) { addBlendShapesMask[i] = false; break; }
                }
            }

            addBlendShapesList.BuildList(blendShapesOnSkin);
            addBlendShapesList.MaskEntries(addBlendShapesMask);
        }

        private void AddBlendShapeToData(int obj)
        {
            blendShapeData.blending.AddBlendShape(obj, new Spline1(5, CurveType.CatmullRom));
            UpdateBlendShapeSplines();
        }

        public void UpdateBlendShapeSplines()
        {
            blendShapeSplines.Clear();
            blendShapeDrawingElements = new Spline1DrawingElement[blendShapeData.blending.blendShapeSplines.Length];

            for (int i = 0; i < blendShapeData.blending.blendShapeSplines.Length; i++)
            {
                int id = i;
                VisualElement bspe = new VisualElement();

                VisualElement labelBar = new VisualElement();
                labelBar.style.flexDirection = FlexDirection.Row;
                bspe.Add(labelBar);

                Label bspl = new Label(blendShapeData.blending.blendShapeNames[i]);
                labelBar.Add(bspl);

                Button removebutton = new Button();
                removebutton.AddToClassList("icon");
                removebutton.AddToClassList("remove");
                removebutton.clicked += () => { blendShapeData.blending.RemoveBlendShape(id); UpdateBlendShapeSplines(); };
                labelBar.Add(removebutton);

                VisualElement bsps = new VisualElement();
                bsps.style.borderBottomWidth = 5;
                bsps.style.borderLeftWidth = 5;
                bsps.style.borderRightWidth = 5;
                bsps.style.borderTopWidth = 5;
                bsps.style.minHeight = 40;
                bspe.Add(bsps);

                Spline1DrawingElement blendShapeSpline = new Spline1DrawingElement(blendShapeData.blending.blendShapeSplines[i]);
                blendShapeSpline.OnSetPoint += evt => { blendShapeData.SetCurrentBlend(blendShapeData.CurrentBlendPercent); blendShapeData.UpdateCurrentBlend(); };
                bsps.Add(blendShapeSpline);

                blendShapeDrawingElements[i] = blendShapeSpline;

                blendShapeSplines.Add(bspe);
            }

        }
    }
}