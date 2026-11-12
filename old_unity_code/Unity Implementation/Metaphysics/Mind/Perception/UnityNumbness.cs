using System;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    [Serializable]
    public class UnityNumbness : UnitySenseNoise<UnityInteroception, UnitySenseOfSelf>
    {
        public override UnitySenseOfSelf ApplyNoise(UnitySenseOfSelf signal)
        {
            throw new NotImplementedException();
        }
    }
}