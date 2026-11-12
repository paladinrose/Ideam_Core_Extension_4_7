using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Ideam.Epistemology.Semantics;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Science.Applied.Cartography
{
    public interface IGeneralization
    {
        TFeature[] Generalize<TFeature, TThing>(TThing[] thingsToGeneralize)
            where TFeature : IFeature
            where TThing : IThing;
    }
}
