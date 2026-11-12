using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Science.Formal.Graphs;

#if UNITY_EDITOR
using UnityEditor;
using UnityEditor.Experimental.GraphView;

namespace Ideam.Unity.Science.Applied.Architecture
{
    public class BlueprintsWindow : EditorWindow
    {
        public static BlueprintsWindow OpenBlueprint(Blueprint blueprint)
        {
            BlueprintsWindow bpw = CreateWindow<BlueprintsWindow>(blueprint.name);
            
            bpw.blueprint = blueprint;


            bpw.Show();
            
            return bpw;
        }
        

        Blueprint blueprint;
        StyleSheet blueprintStyle;

        VisualElement blueprintElement, blueprintContent;
        protected virtual void OnEnable()
        {
            blueprintStyle = AssetDatabase.LoadAssetAtPath<StyleSheet> ("Assets/Ideam/Unity Implementation/UI/Blueprint.uss");
            
        }
        public void DrawBlueprints()
        {
        }
    }


  
}
#endif