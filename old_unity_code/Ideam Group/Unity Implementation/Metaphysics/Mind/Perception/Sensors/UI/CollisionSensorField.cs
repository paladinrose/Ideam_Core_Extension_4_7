using Ideam.Unity.UI;

using UnityEngine.UIElements;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    public class CollisionSensorField : BaseSensorField<CollisionSensor>
    {
        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }


            return true;
        }
    }
}
