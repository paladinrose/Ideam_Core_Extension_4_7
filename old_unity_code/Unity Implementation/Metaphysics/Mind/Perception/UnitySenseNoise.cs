
using Ideam.Science.Formal.Mathematics.Information;
using Ideam.Unity.Science.Formal.Mathematics.Information;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    public abstract class UnitySenseNoise<TSense, TSignal> : INoise
        where TSense : UnitySense<TSignal>
        where TSignal : UnitySignal
    {
        public virtual  T ApplyNoise<T>(T signal) where T : ISignal
        {
            if(signal is TSignal tSignal) 
            {
                if (ApplyNoise(tSignal) is T sig) { return sig; }
            }
            return default;
        }
        public abstract TSignal ApplyNoise(TSignal signal);
    }
}