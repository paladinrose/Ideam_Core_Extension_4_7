using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Applied.Architecture;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Science.Applied.Architecture
{

    public class Structure : UnityGamePiece, IStructure
    {
        public T[] Components<T>()
        {
            List<T> comps = new List<T>();
            for (int i = 0; i < subPieces.Length; i++)
            {
                if (subPieces[i] is T tc) { comps.Add(tc); }

            }
            return comps.ToArray();
        }
    }
}