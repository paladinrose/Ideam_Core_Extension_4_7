using UnityEditor;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;

namespace Ideam.Editor.Sensors
{
    [CustomEditor(typeof(CollisionSensor))]
    public class CollisionSensorEditor : BaseSensorEditor<CollisionSensor, CollisionSensorField>
    {
        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            //excluded = excluded.Concat(new string[] { }).ToArray();
            return excluded;
        }
    }
}