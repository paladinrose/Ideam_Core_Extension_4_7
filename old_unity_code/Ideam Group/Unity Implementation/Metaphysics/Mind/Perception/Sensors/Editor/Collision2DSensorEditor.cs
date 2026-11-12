using UnityEditor;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;

namespace Ideam.Editor.Sensors
{
    [CustomEditor(typeof(Collision2DSensor))]
    public class Collision2DSensorEditor : BaseSensorEditor<Collision2DSensor, Collision2DSensorField>
    {
        protected override string[] ExcludedProperties()
        {
            string[] excluded = base.ExcludedProperties();
            //excluded = excluded.Concat(new string[] { }).ToArray();
            return excluded;
        }
    }
}