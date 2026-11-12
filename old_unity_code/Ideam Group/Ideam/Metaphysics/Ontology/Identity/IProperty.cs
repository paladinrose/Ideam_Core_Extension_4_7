using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;

namespace Ideam.Metaphysics.Ontology.Identity
{
    public interface IProperty
    {
        int Sememe();
        T Value<T>() where T : IValue<T>;
    }
}
