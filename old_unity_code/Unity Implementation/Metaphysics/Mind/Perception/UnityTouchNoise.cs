using System;
namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    [Serializable]
    public class UnityTouchNoise : UnitySenseNoise<UnityTouch, UnityHapticSignal>
    {
        public override UnityHapticSignal ApplyNoise(UnityHapticSignal signal)
        {
            throw new NotImplementedException();
        }
    }
}