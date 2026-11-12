using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

using Ideam.Unity.UI;
using Ideam.Editor;
using Ideam.Unity;

using UnityEngine.UIElements;
using System;
using Ideam.Unity.Semantics;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    [CustomEditor(typeof(UnityGamePiece))]
    public class UnityGamePieceEditor : UnityGamePieceEditor<UnityGamePiece, UnityGamePieceUI> { }

    public class UnityGamePieceEditor<T, TUI> : EditorUserInterface<T, TUI>
        where T:UnityGamePiece
        where TUI : UnityGamePieceUI<T>
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[]
            {
                "properties", "resources", "game", "gameAgent", "gameBoard", "subPieces", "semanticManager", "languageAsset",
                
            }).ToArray();
        }

        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            ui.OnDefaultUXMLLoaded += evt =>
            {
                ui.ShowGameInfoFoldout = true;
                //ui.OnAddProperty += evt => { editorObject.AddSememe(evt); };
                //ui.OnAddResource += evt => { editorObject.AddResource(evt); };
                ui.AllowAddingSubPieces = true;
                ui.AllowRemovingSubPieces = true;

                ui.AllowAddingProperties = true;
                ui.AllowRemovingProperties = true;

                ui.AllowAddingResources = true;
                ui.AllowRemovingResources = true;
            };
        }

        protected override void UpdateObject()
        {
            base.UpdateObject();
            if (editorObject.languageAsset) { EditorUtility.SetDirty(editorObject.languageAsset); }
            if (editorObject.semanticManager) { EditorUtility.SetDirty(editorObject.semanticManager); }
            AssetDatabase.SaveAssets();
        }
    }
}