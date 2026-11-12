using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;

namespace Ideam.Science.Formal.Mathematics.Information
{
    public interface ISignal
    {
        T PercentTransmitted<T, TValue>() where T : IValue<TValue>;

        T WholeSignal<T>();

        T CurrentSignal<T>();

        void Update<T, TValue>(T delta) where T : IValue<TValue>;
    }
}
