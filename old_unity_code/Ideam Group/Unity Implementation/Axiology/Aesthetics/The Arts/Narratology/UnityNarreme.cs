using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;
using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Axiology.Valuation;
using Ideam.Metaphysics.Ontology.Identity;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    
    public class UnityNarreme : ScriptableObject, INarreme, IThing
    {
        public IntAffinity semantics;

        public NarrativeProperty[] narrativeProperties = new NarrativeProperty[0];
        
        public UnityNarrative narrative;


        #region IThing Methods

        public virtual T[] Properties<T>() where T : IProperty
        {
            List<T> tProps = new List<T>();
            for(int i =0; i < narrativeProperties.Length; i++)
            {
                if(narrativeProperties[i] is T tProp) { tProps.Add(tProp); }
            }
            return tProps.ToArray();
        }

        #endregion

        public virtual ConditionStatus CheckNarrativeCondition<T>(int condition, T narremeToCheck = default) where T : INarreme
        { return ConditionStatus.NotMet; }
        
        public virtual T[] GetSymbolicForms<T>() where T:INarreme 
        { return new T[0]; }

        public virtual T GetLiteralForm<T>() where T:INarreme 
        { return default; }

    }

    [Serializable]
    public struct NarrativeProperty : IProperty
    {
        public int sememe;
        public Int value;
        public int Sememe()
        {
            return sememe;
        }

        public T Value<T>() where T : IValue<T>
        {
            if(value is T tValue) { return tValue; }
            return default;
        }
    }
}
