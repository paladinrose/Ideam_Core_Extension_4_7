using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Natural.Physics;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Axiology.Valuation;

namespace Ideam.Science.Formal.Mathematics
{
   public interface IArea<TBounds, TValue> : IPlace 
        where TValue:IValue<TValue> 
        where TBounds:IBoundary<TBounds, TValue>
   {
        TBounds Bounds { get; }

        bool Contains<T>(T point) where T : IDimensional<TValue>;
    }
}