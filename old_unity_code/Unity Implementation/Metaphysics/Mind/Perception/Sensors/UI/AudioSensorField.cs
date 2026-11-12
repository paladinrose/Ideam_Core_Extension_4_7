using Ideam.Unity.UI;

using UnityEngine.UIElements;

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    [UserInterface(typeof(AudioSensor))]
    public class AudioSensorField : BaseSensorField<AudioSensor>
    {
        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }


            return true;
        }
    }
}
