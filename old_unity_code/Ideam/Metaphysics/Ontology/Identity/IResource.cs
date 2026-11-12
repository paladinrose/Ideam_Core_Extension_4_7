using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;

namespace Ideam.Metaphysics.Ontology.Identity
{
    public interface IResource : IProperty
    {
        T MaximumValue<T>() where T : IValue<T>;

        T UseResource<T>() where T : IValue<T>;
    }
}
