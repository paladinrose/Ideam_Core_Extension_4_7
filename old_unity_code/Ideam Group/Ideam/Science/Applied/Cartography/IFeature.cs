using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Ideam.Axiology.Valuation;
using Ideam.Epistemology.Semantics;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Science.Applied.Cartography
{
    public interface IFeature: ISemantic
    {
        Type Represents();

        T Subject<T>() where T:IThing;

        T[] CoordinatesFootprint<T>() where T : IValue<T>;

    }
}
