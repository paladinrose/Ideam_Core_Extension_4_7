using System.Collections.Generic;

using Ideam.Science.Applied.Architecture;
using Ideam.Metaphysics.Ontology.Existence;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Science.Applied.Architecture
{
    public class Wall : UnityGamePiece, IWall
    {
        public Surface[] surfaces;

        public UnityGamePlace[] walledOffPlaces;

        public T[] Surfaces<T>() where T : ISurface
        {
            List<T> surfacesList = new List<T>();
            for (int i = 0; i < surfaces.Length; i++)
            {
                if (surfaces[i] is T tsurface) { surfacesList.Add(tsurface); }
            }

            return surfacesList.ToArray();
        }

        public T[] WalledOffPlaces<T>() where T : IPlace
        {
            List<T> placesList = new List<T>();
            for (int i = 0; i < walledOffPlaces.Length; i++)
            {
                if (walledOffPlaces[i] is T tplace) { placesList.Add(tplace); }
            }

            return placesList.ToArray();
        }
    }
}