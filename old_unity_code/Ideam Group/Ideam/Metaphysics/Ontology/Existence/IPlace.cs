using System;
using System.Collections.Generic;
using System.Text;
using Ideam.Science.Natural.Physics;

namespace Ideam.Metaphysics.Ontology.Existence
{
    /// <summary>
    /// The most nondescript part of a space. It may be boundless or finite, literal and/or
    /// metaphysical.
    /// </summary>
    public interface IPlace
    {
        T[] GetThingsWithin<T>() where T : IThing;
    }
}
