using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using UnityEngine.UIElements;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Editor;
using Ideam.Unity.UI;

namespace Ideam.Editor.Sensors
{
    public abstract class BaseSensorEditor<T, TField> : EditorUserInterface<T, TField>
        where T:BaseSensor
        where TField : BaseSensorField<T>
    {

        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            excluded = excluded.Concat(new string[] { "isSensing"}).ToArray();
            return excluded;
        }
        
        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            ui.baseSensorUXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Metaphysics/Mind/Perception/Sensors/UI/BaseSensor.uxml");
        }
        
    }
}