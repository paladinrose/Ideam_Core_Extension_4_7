using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Epistemology.Semantics;
using System;

//BIG NOTE TO SELF: SememeID 0 == NOTHING. ZERO. NADA. ZIP. ABSENCE. VOID.
//Negative values are totally fine to use.
namespace Ideam.Unity.Semantics
{
    [CreateAssetMenu(menuName ="Ideam/Semantics/Semantic Manager")]
    public class UnitySemanticManager : ScriptableObject, ISemanticManager
    {

        public Sememe[] sememes = new Sememe[0];

        [SerializeField, HideInInspector]
        protected int semeCount;

        public int[] GetConnotation(int sememeID, int[] context)
        {
            throw new System.NotImplementedException();
        }

        public int[] GetDenotation(int sememeID)
        {
            throw new System.NotImplementedException();
        }

        public T GetSememe<T>(int sememeID) where T : ISememe
        {
            throw new System.NotImplementedException();
        }

        public Sememe GetSememe(int sememeID)
        {
            throw new System.NotImplementedException();
        }

        public int GetSememeID<T>(T sememe) where T : ISememe
        {
            if(sememe is Sememe s) { return GetSememeID(s); }
            return -1;
        }

        public int GetSememeID(Sememe sememe)
        {
            
            for(int i =0; i < sememes.Length; i++)
            {
                if(sememes[i].semes.Length != sememe.semes.Length) { continue; }

                bool thisIsIt = true;
                for(int j =0; j < sememes[i].semes.Length; j++)
                {
                    if(sememes[i].semes[j] != sememe.semes[j]) { thisIsIt = false; break; }
                }
                if (thisIsIt) { return i; }
            }

            int newSememeID = sememes.Length;
            System.Array.Resize(ref sememes, newSememeID + 1);
            sememes[newSememeID] = sememe;
            return newSememeID;
        }

        public T[] GetSememes<T>(int[] sememeIDs) where T : ISememe
        {
            throw new System.NotImplementedException();
        }
        public Sememe[] GetSememes(int[] sememeIDs)
        {
            throw new System.NotImplementedException();
        }

        public int NewSeme()
        {
            semeCount++;
            return semeCount-1;
        }
    }
}
