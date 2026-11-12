using UnityEditor;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;

namespace Ideam.Editor.Sensors
{
    [CustomEditor(typeof(Trigger2DSensor))]
    public class Trigger2DSensorEditor : BaseSensorEditor<Trigger2DSensor, Trigger2DSensorField>
    {
        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            //excluded = excluded.Concat(new string[] { }).ToArray();
            return excluded;
        }
    }
}
