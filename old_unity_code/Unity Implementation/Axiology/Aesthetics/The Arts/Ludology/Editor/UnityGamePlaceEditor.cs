using System.Linq;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEditor;


using Ideam.Editor;
using Ideam.Unity;
using System;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    [CustomEditor(typeof(UnityGamePlace))]
    public class UnityGamePlaceEditor : UnityGamePieceEditor<UnityGamePlace, UnityGamePlaceUI>
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[]
            {
                "placeBounds"
            }).ToArray();
        }

       

        public override VisualElement CreateInspectorGUI()
        {
            base.CreateInspectorGUI();
            
            return root;
        }

       

        protected override void OnSceneGUI(SceneView sceneView)
        {
            if(ui.drawBounds.value) EditorExtension.DrawWireBounds(editorObject.placeBounds, editorObject.transform.position, editorObject.transform.rotation);
            
        }
    }
}