using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Reason
{
    #region Interfaces
    
    /// <summary>
    /// Represents a semantic premise used in Abductive Reasoning
    /// </summary>
    public interface IObservation<T> : IPremise<T>
    {
        TVal Clarity<TVal>() where TVal : IValue<TVal>;
    }

    /// <summary>
    /// Represents a semantic conclusion used in Abductive Reasoning.
    /// </summary>
    public interface IHypothesis<T> : IConclusion<T>
    {
        int TheoryGroup();
    }

    /// <summary>
    /// Represents an argument where a collection of observations contribute to a confidence level in the conclusion.
    /// </summary>
    /// <typeparam name="TObservation"></typeparam>
    /// <typeparam name="THypothesis"></typeparam>
    /// <typeparam name="TProposition"></typeparam>
    public interface IAbduction<TObservation, THypothesis, TInference, TProposition> : IArgument<Int, TObservation, THypothesis, TInference, TProposition>
                          where TObservation : IObservation<TObservation>
                          where THypothesis : IHypothesis<THypothesis>
                          where TInference : IInference<TObservation, THypothesis>
                          where TProposition : IProposition<TObservation, TInference, THypothesis>
    {
        TVal Consistency<TVal>() where TVal : IValue<TVal>;
    }

    #endregion

    #region Structs

    /// <summary>
    /// Represents an observational premise used in Abductive Reasoning.
    /// </summary>
    [Serializable]
    public struct Observation : IObservation<Observation>
    {
        public Int sememe, clarity;
        
        public TVal Sememe<TVal>()where TVal : IValue<TVal>
        {
            if (sememe is TVal tSememe) return tSememe;
            return default;
        }

        public Int Sememe() { return sememe; } 

        public TVal Clarity<TVal>() where TVal : IValue<TVal>
        {
            if(clarity is TVal tClarity) return tClarity; 
            return default;
        }

        public Int Clarity() {  return clarity; }

        public Observation(int sememe, int clarity)
        {
            this.sememe = sememe;
            this.clarity = clarity;
        }

        public bool Equals(Observation otherThought)
        {
            return false;
        }
    }

    /// <summary>
    /// Represents an observational premise used in Abductive Reasoning.
    /// </summary>
    [Serializable]
    public struct Hypothesis : IHypothesis<Hypothesis>
    {
        public Int sememe;
        public int theoryGroup;
        
        public TVal Sememe<TVal>() where TVal : IValue<TVal>
        {
            if (sememe is TVal tSememe) return tSememe;
            return default;
        }
        public Int Sememe() { return sememe; }

        public int TheoryGroup() { return theoryGroup; } 

        public Hypothesis(int sememe, int theoryGroup)
        {
            this.sememe = sememe;
            this.theoryGroup = theoryGroup;
        }

        public bool Equals(Hypothesis otherThought)
        {
            return false;
        }
    }

    public struct AbductiveInference : IInference<Observation, Hypothesis>
    {
        public Hypothesis Infer(Observation A, Observation B)
        {
            throw new System.NotImplementedException();
        }
    }

    [Serializable]
    public struct AbductiveProposition : IProposition<Observation, AbductiveInference, Hypothesis>
    {
        public Observation Antecedent => throw new System.NotImplementedException();

        public AbductiveInference Copula => throw new System.NotImplementedException();

        public Observation Consequent => throw new System.NotImplementedException();

        public Hypothesis Conclusion => throw new System.NotImplementedException();

        public Bool IsTrue => throw new System.NotImplementedException();
    }

    [Serializable]
    public struct AbductiveArgument : IAbduction<Observation, Hypothesis, AbductiveInference, AbductiveProposition>
    {
        public Int consistency, soundness;

        public TVal Consistency<TVal>() where TVal:IValue<TVal>
        {
            if (consistency is TVal tConsistency) return tConsistency;
            return default;
        }
        public Int Consistency() { return consistency; }

        public Int Soundness { get { return soundness; } }

        public void BuildArgument(AbductiveProposition[] premises, Hypothesis conclusion)
        {

        }
    }

    #endregion

}
