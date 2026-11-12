using System;
namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    [Serializable]
    public class UnityVisualNoise : UnitySenseNoise<UnitySight, UnityVisualSignal>
    {
        public override UnityVisualSignal ApplyNoise(UnityVisualSignal signal)
        {
            throw new NotImplementedException();
        }
    }
}