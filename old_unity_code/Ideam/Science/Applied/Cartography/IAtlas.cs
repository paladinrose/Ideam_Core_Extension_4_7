using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Ideam.Science.Applied.Cartography
{
    public interface IAtlas
    {
        T GetMap<T>(int mapID) where T : IMap;
        T[] GetMaps<T>() where T : IMap;
    }
}
