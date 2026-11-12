using System;
using System.Linq;

using UnityEngine.UIElements;
using UnityEditor;

using Ideam.Editor;

namespace Ideam.Unity.Science.Natural.Cosmology 
{
    [CustomEditor(typeof(UnityCosmos))] 
    public class UnityCosmosEditor : EditorUserInterface<UnityCosmos, UnityCosmosUI> 
    {
        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            excluded = excluded.Concat(new string[] 
            {
                "games", "cosmicCamera", "jukebox", "startupGame", "languageAsset", "cosmicObjects"
            }).ToArray();
            return excluded;
        }


        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            //ui.UXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Science/Natural/Cosmology/UI/UnityCosmos.uxml");
            ui.selectedGameUI = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Science/Natural/Cosmology/UI/UnityCosmos_SelectedGame.uxml");
        }
    } 
}