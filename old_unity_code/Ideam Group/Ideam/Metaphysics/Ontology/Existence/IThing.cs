using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Metaphysics.Ontology.Action;
using Ideam.Metaphysics.Ontology.Identity;

namespace Ideam.Metaphysics.Ontology.Existence
{
    public interface IThing
    {
        T[] Properties<T>() where T : IProperty;
    }
}
