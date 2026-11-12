using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Metaphysics.Ontology.Identity;

namespace Ideam.Metaphysics.Ontology.Existence
{
    public interface IReality
    {
        T[] GetThings<T>() where T : IThing, IReal;
        bool IsRealThing<T>(T realityCheck) where T : IThing, IReal;

        T[] GetPlaces<T>() where T : IPlace, IReal;
        bool IsRealPlace<T>(T realityCheck) where T : IPlace, IReal;

        T[] GetProperties<T>() where T : IProperty;
        bool IsRealProperty<T>(T realityCheck) where T : IProperty;

    }
}
