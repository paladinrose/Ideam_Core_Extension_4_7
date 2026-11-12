using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Information;
using Ideam.Axiology.Valuation;

namespace Ideam.Unity.Science.Formal.Mathematics.Information
{
    public abstract class UnitySignal : ISignal
    {
        public Float percentTransmitted;
        public int[] signalSememes;
        public virtual T CurrentSignal<T>()
        {
            int[] cSig = CurrentSignal();
            if(cSig is T tSig) { return tSig; }
            return default;
        }

        public virtual int[] CurrentSignal()
        {
            return new int[0];
        }

        public virtual T PercentTransmitted<T, TValue>() where T : IValue<TValue>
        {
            if(percentTransmitted is T tPer) { return tPer; }
            return default;
        }

        public virtual Float PercentTransmitted()
        {
            return percentTransmitted;
        }

        public void Update<T, TValue>(T delta) where T : IValue<TValue>
        {
            throw new System.NotImplementedException();
        }

        public virtual T WholeSignal<T>()
        {
            if(signalSememes is T tSememes) { return tSememes; }
            return default;
        }
        public virtual int[] WholeSignal()
        {
            return signalSememes;
        }
    }
}