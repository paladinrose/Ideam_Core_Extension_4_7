using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;

namespace Ideam.Science.Formal.Mathematics.Information
{
    public interface ITransmitter
    {
        void BeginTransmission<T>(T signal) where T : ISignal;
        void Transmit<T, TValue, TSignal>(TSignal signal, T deltaTime) where T : IValue<TValue> where TSignal :ISignal;
        T[] GetSignalsTransmitting<T>() where T : ISignal;
        T GetSignal<T>(int id) where T : ISignal;
        void EndTransmission<T>(T signal) where T : ISignal;
    }
}
