using System.Linq;
using UnityEditor;
using UnityEngine.UIElements;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;

namespace Ideam.Editor.Sensors
{
    [CustomEditor(typeof(JointSensor))]
    public class JointSensorEditor : BaseSensorEditor<JointSensor, JointSensorField>
    {
        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            excluded = excluded.Concat(new string[]
            {"JointBreak"}).ToArray();
            return excluded;
        }

        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            ui.UXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam/Unity Implementation/Sensors/UI/JointSensor.uxml");
        }
    }
}