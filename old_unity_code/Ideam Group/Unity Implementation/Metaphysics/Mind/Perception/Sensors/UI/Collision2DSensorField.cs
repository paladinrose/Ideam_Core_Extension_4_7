using Ideam.Unity.UI;

using UnityEngine.UIElements;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    public class Collision2DSensorField : BaseSensorField<Collision2DSensor>
    {
        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }


            return true;
        }
    }
}
