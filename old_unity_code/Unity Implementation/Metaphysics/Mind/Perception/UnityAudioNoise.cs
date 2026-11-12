using System;
namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    [Serializable]
    public class UnityAudioNoise : UnitySenseNoise<UnityHearing, UnityAudioSignal>
    {
        public override UnityAudioSignal ApplyNoise(UnityAudioSignal signal)
        {
            throw new NotImplementedException();
        }
    }
}