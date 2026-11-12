using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;

namespace Ideam.Science.Natural.Physics
{
    public interface IDimensional<T>
    {
        uint Dimensionality { get; }

        void SetDimension(int dimension,  T value);

        T GetDimension(int dimension);
    }

}
