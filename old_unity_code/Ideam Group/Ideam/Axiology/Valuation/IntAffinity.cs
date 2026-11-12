using System;
using System.Collections.Generic;

using Ideam.Epistemology.Semantics;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Axiology.Valuation
{
    [Serializable]
    public class IntAffinity : IAffinity
    {
        #region IAffinity Implementation

        public int[] sememes;
        public Int[] semanticAffinities;

        public int semanticManagerID;

        SemanticManager semanticManager = null;

        public virtual T[] GetSemanticAffinities<T>() where T : IValue<T>
        {
            if (typeof(T).IsAssignableFrom(typeof(Int)))
            {
                return semanticAffinities as T[];
            }
            return new T[0];
        }

        public virtual T GetSememeAffinity<T>(int localID) where T : IValue<T>
        {
            if (localID < 0 || localID >= sememes.Length) { return default; }
            if (semanticAffinities[localID] is T tSem) { return tSem; }
            return default;
        }

        public virtual void SetSememeAffinity<T>(int localID, T newAffinity) where T : IValue<T>
        {
            if (localID < 0 || localID >= sememes.Length) { return; }
            if (newAffinity is Int newInt)
            {
                semanticAffinities[localID] = newInt;
            }
        }
        
        public virtual T GetSemanticManager<T>() where T : ISemanticManager
        {
            
            if (semanticManager is T semMan)
            {
                return semMan;
            }
            return default;
        }
        
        public virtual T[] GetSememes<T>() where T : ISememe
        {
            return new T[0];
            //return semanticManager.GetSememes<T>(sememes);
        }
        public virtual int AddSememe(int globalSememeID)
        {
            int id = GetLocalSememeID(globalSememeID);
            if(id >= 0) { return id; }

            id = sememes.Length;
            System.Array.Resize(ref sememes, id + 1);
            sememes[id] = globalSememeID;
            return id;
        }

        public virtual int AddSemanticAffinity<T>(int globalSememeID, T affinity)
        {
            int id = GetLocalSememeID(globalSememeID);
            if (id >= 0) { return id; }

            id = sememes.Length;
            System.Array.Resize(ref sememes, id + 1);
            System.Array.Resize(ref semanticAffinities, id + 1);
            sememes[id] = globalSememeID;
            if(affinity is Int intAff)
            {
                semanticAffinities[id] = intAff;
            }
            return id;
        }

        public virtual int GetSememeID(int localID)
        {
            return sememes[localID];
        }

        public int[] GetSememeIDs() { return sememes; }

        public virtual int GetLocalSememeID(int globalSememeID)
        {
            for (int i = sememes.Length - 1; i >= 0; i--)
            {
                if (sememes[i] == globalSememeID) return i;
            }
            return -1;
        }

        public virtual void SetSememeAtID(int newSememeID, int id)
        {
            sememes[id] = newSememeID;
        }

        public virtual void RemoveSememe(int toRemove)
        {
            if (toRemove < 0 || toRemove >= sememes.Length) { return; }
            List<int> newSememes = new List<int>(sememes);
            List<Int> newAffinities = new List<Int>(semanticAffinities);

            newSememes.RemoveAt(toRemove);
            newAffinities.RemoveAt(toRemove);

            sememes = newSememes.ToArray();
            semanticAffinities = newAffinities.ToArray();
        }

        public virtual void CompareSemantics(ISemantic other, out int[] sharedSememes, out int[] othersMissingSememes)
        {
            List<int> shared = new List<int>();
            List<int> notShared = new List<int>();
            int[] oC = other.GetSememeIDs();
            for (int i = sememes.Length - 1; i >= 0; i--)
            {
                bool has = false;
                for (int j = oC.Length; j >= 0; j--)
                {
                    if (oC[j] == sememes[i])
                    {
                        has = true; break;
                    }
                }
                if (has)
                {
                    shared.Add(i);
                }
                else
                {
                    notShared.Add(i);
                }
            }
            othersMissingSememes = notShared.ToArray();
            sharedSememes = shared.ToArray();
        }

        public virtual bool CompareSemanticAffinity<T>(IAffinity other, out T degreeOfDifference)
            where T : IValue<T>
        {
            if (typeof(T) != typeof(Int)) { degreeOfDifference = default; return false; }

            T score = default;
            CompareSemantics(other, out int[] qualities, out int[] lackingQualities);

            for (int j = lackingQualities.Length - 1; j >= 0; j--)
            {
                score = score.Add(GetSememeAffinity<T>(lackingQualities[j]).Abs());
            }

            for (int j = qualities.Length - 1; j >= 0; j--)
            {
                int qid = sememes[qualities[j]];
                T siteScore = other.GetSememeAffinity<T>(qid).Subtract(
                                GetSememeAffinity<T>(qualities[j]));
                score = score.Add(siteScore.Abs());
            }

            degreeOfDifference = score;
            return true;
        }
        #endregion

    }
}
