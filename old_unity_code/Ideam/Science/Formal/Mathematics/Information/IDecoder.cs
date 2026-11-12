using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Mathematics.Information
{
    public interface IDecoder
    {
        TDecoded DecodeSignal<TSignal, TDecoded>(TSignal signalToDecode) where TSignal :ISignal;
    }
}
