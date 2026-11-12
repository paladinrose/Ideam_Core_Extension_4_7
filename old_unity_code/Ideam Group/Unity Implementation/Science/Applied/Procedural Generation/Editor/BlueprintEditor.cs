using System;
using System.Linq;
using System.Collections;

using UnityEngine;
using UnityEngine.UIElements;

using UnityEditor;
using UnityEditor.UIElements;

using Ideam.Unity.UI;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Editor;


using ProgressBar = Ideam.Unity.UI.ProgressBar;

namespace Ideam.Unity.ProceduralGeneration
{
    [CustomEditor(typeof(Blueprint))]
    public class BlueprintEditor : UIElementsEditor
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[]
            {
            }).ToArray();
        }

        protected Blueprint blueprint;

        protected override void OnEnable()
        {
            base.OnEnable();
            blueprint = (Blueprint)target;
        }

        protected override void OnDisable()
        {
            base.OnDisable();
        }

        public override VisualElement CreateInspectorGUI()
        {
            base.CreateInspectorGUI();


            return root;
        }


    }
}