using UnityEditor;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;

namespace Ideam.Editor.Sensors
{
    [CustomEditor(typeof(InputSensor))]
    public class InputSensorEditor : BaseSensorEditor<InputSensor, InputSensorField>
    {
        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            //excluded = excluded.Concat(new string[] { }).ToArray();
            return excluded;
        }
    }
}