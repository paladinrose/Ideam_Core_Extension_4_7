using System;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Science.Applied.Architecture
{

    public interface IDoor : IAperture
    {
        
        event Action<IThing> OnTraverse;

        bool Traverse<T>(T thing, int fromPlace, int toPlace) where T:IThing;
    }
}