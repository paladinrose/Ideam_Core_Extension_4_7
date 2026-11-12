using System.Collections.Generic;
using Ideam.Science.Applied.Architecture;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Science.Applied.Architecture
{
    public class Ceiling : UnityGamePiece, ICeiling
    {
        public Surface[] surfaces;
        public Floor floorAbove;

        public T[] CeilingSurfaces<T>() where T : ISurface
        {
            List<T> surfacesList = new List<T>();
            for (int i = 0; i < surfaces.Length; i++)
            {
                if (surfaces[i] is T tsurface) { surfacesList.Add(tsurface); }
            }

            return surfacesList.ToArray();
        }

        public T FloorAbove<T>() where T : IFloor
        {
            if (floorAbove is T tAbove) { return tAbove; }
            return default;
        }
    }
}