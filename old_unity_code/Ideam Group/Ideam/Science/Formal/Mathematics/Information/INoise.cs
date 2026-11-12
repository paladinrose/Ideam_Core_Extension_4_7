using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Mathematics.Information
{
    public interface INoise
    {
        T ApplyNoise<T>(T signal) where T : ISignal;
    }
}
