using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using UnityEditor;

using Ideam.Unity.UI;

using Ideam.Unity.Science.Formal.Mathematics;

using Ideam.Editor;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    [CustomEditor(typeof(AnimatorControl))]
    public class AnimatorControlEditor : BaseControlEditor<AnimatorControl, AnimatorControlUI>
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[] 
            {
                "blendShapeData"
            }).ToArray();
        }

        public override VisualElement CreateIdeamGUI()
        {
            root = ui.OpenUserInterface();
            root.Insert(0, CreateDefaultInspectorGUI());
            root.style.minHeight = 100;
            return root;
        }
        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            ui.UXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Axiology/Aesthetics/The Arts/Ludology/Gameplay/Controls/UI/AnimatorControl.uxml");
        }
    }

}