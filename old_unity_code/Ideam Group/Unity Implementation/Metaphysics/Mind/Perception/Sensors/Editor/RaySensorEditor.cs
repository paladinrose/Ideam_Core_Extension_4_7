using System.Linq;
using UnityEditor;

using UnityEngine.UIElements;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;

namespace Ideam.Editor.Sensors
{
    [CustomEditor(typeof(RaySensor))]
    public class RaySensorEditor : BaseSensorEditor<RaySensor, RaySensorField>
    {
        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            excluded = excluded.Concat(new string[] 
            {"rays" }).ToArray();
            return excluded;
        }

        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            ui.UXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Metaphysics/Mind/Perception/Sensors/UI/RaySensor.uxml");
            ui.rayUXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Metaphysics/Mind/Perception/Sensors/UI/SensorRay.uxml");
        }
    }
}