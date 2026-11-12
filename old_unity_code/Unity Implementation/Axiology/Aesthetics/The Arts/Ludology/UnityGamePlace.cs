using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class UnityGamePlace : UnityGamePiece, IPlace
    {
        public Bounds placeBounds;
        public override void GameStart(IGame game)
        {
            base.GameStart(game);
            placeBounds = transform.GlobalBounds(true);
        }

        public T[] GetThingsWithin<T>() where T : IThing
        {
            List<T> newThings = new List<T>();
            for(int i =0; i < subPieces.Length; i++)
            {
                if (!placeBounds.Contains(subPieces[i].transform.position)) { continue; }

                if(subPieces[i] is T tComp)
                {
                    newThings.Add(tComp);  
                }
            }
            return newThings.ToArray();
        }
    }
}