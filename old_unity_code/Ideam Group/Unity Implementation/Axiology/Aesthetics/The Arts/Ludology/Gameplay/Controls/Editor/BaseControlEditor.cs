using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;
using UnityEditor;

using Ideam.Unity.UI;
using Ideam.Editor;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class BaseControlEditor<T, TUI> : EditorUserInterface<T, TUI>
        where T:BaseControl
        where TUI : BaseControlUI<T>
    {
        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            //ui.baseControlUXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Axiology/Aesthetics/The Arts/Ludology/Gameplay/Controls/UI/BaseControl.uxml");
        }
    }
}