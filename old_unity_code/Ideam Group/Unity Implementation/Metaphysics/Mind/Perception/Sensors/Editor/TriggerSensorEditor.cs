using System.Linq;
using UnityEditor;
using UnityEngine.UIElements;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;

namespace Ideam.Editor.Sensors
{
    [CustomEditor(typeof(TriggerSensor))]
    public class TriggerSensorEditor : BaseSensorEditor<TriggerSensor, TriggerSensorField>
    {
        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            excluded = excluded.Concat(new string[] 
            {"OnFirstTrigger", "OnStartTrigger", "WhileTriggered", 
             "OnStopTrigger", "OnLastTriggerStop", "collidersInTrigger"}).ToArray();
            return excluded;
        }

        protected override void SetupEditorObject()
        {
            base.SetupEditorObject();
            ui.UXML = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam Group/Unity Implementation/Metaphysics/Mind/Perception/Sensors/UI/TriggerSensor.uxml");
        }
    }
}