using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Metaphysics.Ontology.Identity;

namespace Ideam.Metaphysics.Ontology.Existence
{
    public interface IImagination
    {
        T[] GetThings<T>() where T : IThing, IImaginary;
        T[] GetPlaces<T>() where T : IPlace, IImaginary;
        T[] GetProperties<T>() where T : IProperty, IImaginary;
    }
}
