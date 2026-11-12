using System.Collections.Generic;

using Ideam.Science.Applied.Architecture;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Science.Applied.Architecture
{
    public class Floor : UnityGamePiece, IFloor
    {
        public Surface[] surfaces;
        public Ceiling ceilingBelow;

        public T CeilingBelow<T>() where T : ICeiling
        {
            if(ceilingBelow is T tC) { return tC; }
            return default;
        }

        public T[] FloorSurfaces<T>() where T : ISurface
        {
            List<T> floorSurfaces = new List<T>();
            for(int i =0; i < surfaces.Length; i++)
            {
                if(surfaces[i] is T tSurf)
                {
                    floorSurfaces.Add(tSurf);
                }
            }
            return floorSurfaces.ToArray();
        }
    }
}