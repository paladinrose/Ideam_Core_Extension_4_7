using System;
using System.Linq;
using System.Collections;

using UnityEngine;
using UnityEngine.UIElements;

using UnityEditor;
using UnityEditor.UIElements;

using Ideam.Unity.UI;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Editor;


using ProgressBar = Ideam.Unity.UI.ProgressBar;

namespace Ideam.Unity.Science.Applied.Architecture
{
    [CustomEditor(typeof(ArchitecturalPlot))]
    public class ArchitecturalPlotEditor : UnityGamePlaceEditor
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[]
            {
               "blueprint"
            }).ToArray();
        }

        protected ArchitecturalPlot plot;

        protected ObjectSelectorField blueprintSelector;
        protected Button saveBlueprintAssetButton, openBlueprintButton;
        Label warningMessage;
        protected override void OnEnable()
        {
            base.OnEnable();
            plot = (ArchitecturalPlot)target;
            if (!plot.blueprint)
            {
                plot.blueprint = CreateInstance<Blueprint>();
                serializedObject.ApplyModifiedProperties();
            }
            
        }


        public override VisualElement CreateInspectorGUI()
        {
            
            base.CreateInspectorGUI();

            VisualElement blueprintBar = new VisualElement();
            blueprintBar.style.flexDirection = FlexDirection.Row;
            root.Add(blueprintBar);

            blueprintSelector = new ObjectSelectorField("Blueprint Asset");
            blueprintSelector.Initialize<Blueprint>(SelectBlueprintAsset, plot.blueprint);
            blueprintBar.Add(blueprintSelector);

            saveBlueprintAssetButton = new Button(SaveBlueprintAsset);
            saveBlueprintAssetButton.AddToClassList("icon");
            saveBlueprintAssetButton.AddToClassList("save");
            blueprintBar.Add(saveBlueprintAssetButton);
          
            
            openBlueprintButton = new Button(OpenBlueprint);
            openBlueprintButton.text = "Open Blueprint";
            root.Add(openBlueprintButton);

            warningMessage = new Label("Warning");
            root.Add(warningMessage);
            //warningMessage.style.color = Color.red;
            warningMessage.style.display = DisplayStyle.None;
            
            ValidateBlueprintAssetPath();

            return root;
        }

        private void SelectBlueprintAsset(Blueprint selection)
        {
            plot.blueprint = selection;
            serializedObject.ApplyModifiedProperties();
        }

        protected virtual void OpenBlueprint()
        {
            BlueprintsWindow bpw = BlueprintsWindow.OpenBlueprint(plot.blueprint);
            bpw.DrawBlueprints();

        }

        protected virtual void ValidateBlueprintAssetPath()
        {
            string bluePrintPath = AssetDatabase.GetAssetPath(plot.blueprint);
            if (!string.IsNullOrEmpty(bluePrintPath))
            {
                saveBlueprintAssetButton.style.display = DisplayStyle.None;
                warningMessage.text = bluePrintPath;
                warningMessage.style.display = DisplayStyle.Flex;
            }
           
            return;

        }

        protected virtual void SaveBlueprintAsset()
        {
            //Open the asset saving menu.
            //Use Asset Database to save at the path provided.
            //Hide the save button.
            //Apply changes to the serializedObject.

        }

        
    }
}
