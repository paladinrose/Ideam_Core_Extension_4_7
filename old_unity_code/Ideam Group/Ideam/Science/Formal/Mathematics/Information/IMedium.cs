using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Mathematics.Information
{
    public interface IMedium
    {
        T[] GetTransmitters<T>() where T : ITransmitter;
        T[] GetReceivers<T>() where T : IReceiver;
        T[] GetSignalsInTransmission<T>() where T : ISignal;
        T GetNoise<T, TSignal>(TSignal signal) where T : INoise where TSignal : ISignal;
    }
}
