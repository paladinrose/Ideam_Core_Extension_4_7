using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Ideam.Metaphysics.Ontology.Existence;
namespace Ideam.Science.Applied.Cartography
{
    public interface IProjection
    {
        TMap ProjectToMap<TMap, TPlace>(TPlace place)
            where TMap : IMap 
            where TPlace : IPlace;
    }
}
