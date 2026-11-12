using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Formal.Graphs;
namespace Ideam.Science.Applied.Cartography
{
    public interface IMap// : IGraph
    {
        T[] GetFeatures<T>() where T : IFeature;
        
    }
}
