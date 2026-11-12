using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Metaphysics.Mind
{
    public interface IMentalState
    {
        T ExcitationLevel<T, TValue>() where T : IValue<TValue>;

    }
}