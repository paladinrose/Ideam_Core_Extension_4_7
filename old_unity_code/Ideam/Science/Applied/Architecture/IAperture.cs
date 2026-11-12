using System;

using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Science.Applied.Architecture
{
    public interface IAperture
    {
        T[] Surfaces<T>() where T : ISurface;

        T[] LinkedPlaces<T>() where T : IPlace;

        bool IsOpen { get; }
        
        event Action OnOpen, 
                     OnClose;

        bool Open();
        bool Close();

        void UpdateAperture();
    }
}