using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Applied.Architecture;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Science.Applied.Architecture
{
    public class Room : UnityGamePlace, IRoom
    {
        public Surface[] surfaces;

        public Wall[] walls;
        public Floor[] floors;
        public Ceiling[] ceilings;

        public T[] Ceilings<T>() where T : ICeiling
        {
            List<T> ceilingsList = new List<T>();
            for (int i = 0; i < ceilings.Length; i++)
            {
                if (ceilings[i] is T tceiling) { ceilingsList.Add(tceiling); }
            }

            return ceilingsList.ToArray();
        }

        public T[] Floors<T>() where T : IFloor
        {
            List<T> floorsList = new List<T>();
            for (int i = 0; i < floors.Length; i++)
            {
                if (floors[i] is T tfloor) { floorsList.Add(tfloor); }
            }

            return floorsList.ToArray();
        }

        public T[] Surfaces<T>() where T : ISurface
        {
            List<T> surfacesList = new List<T>();
            for (int i = 0; i < surfaces.Length; i++)
            {
                if (surfaces[i] is T tsurface) { surfacesList.Add(tsurface); }
            }

            return surfacesList.ToArray();
        }

        public T[] Walls<T>() where T : IWall
        {
            List<T> wallsList = new List<T>();
            for (int i = 0; i < walls.Length; i++)
            {
                if (walls[i] is T twall) { wallsList.Add(twall); }
            }

            return wallsList.ToArray();
        }
    }
}