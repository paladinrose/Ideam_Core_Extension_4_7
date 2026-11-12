using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Mathematics.Information
{
    public interface IEncoder
    {
        TEncoded EncodeSignal<TSignal, TEncoded>(TSignal signalToEncode) where TEncoded:ISignal;

    }
}
