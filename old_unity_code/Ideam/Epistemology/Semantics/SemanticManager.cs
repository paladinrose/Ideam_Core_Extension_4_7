using System;
using System.Collections.Generic;

namespace Ideam.Epistemology.Semantics
{
    //BIG NOTE TO SELF: SememeID 0 == NOTHING. ZERO. NADA. ZIP. ABSENCE. VOID.
    //Negative values are totally fine to use.
    [Serializable]
    public class SemanticManager : ISemanticManager
    {
        public Sememe[] sememes;
        public int[] GetConnotation(int sememeID, int[] context)
        {
            throw new NotImplementedException();
        }

        public int[] GetDenotation(int sememeID)
        {
            throw new NotImplementedException();
        }

        public T GetSememe<T>(int sememeID) where T : ISememe
        {
            if (sememes[sememeID] is T tSememe)
            {
                return tSememe;
            }
            return default;
        }

        public T[] GetSememes<T>(int[] sememeIDs) where T : ISememe 
        {
            List<T> sememeTs = new List<T>();
            for(int i = 0; i < sememeIDs.Length; i++)
            {
                if(sememes[sememeIDs[i]] is T tSememe) { sememeTs.Add(tSememe); }
            }
            return sememeTs.ToArray();
        }

        public int GetSememeID<T>(T sememe) where T : ISememe
        {
            throw new NotImplementedException();
        }
    }
}
