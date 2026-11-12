using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Science.Applied.Architecture
{
    public interface IWall 
    {
        T[] Surfaces<T>() where T : ISurface;
        T[] WalledOffPlaces<T>() where T : IPlace;
    }

}