using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Axiology.Valuation;

namespace Ideam.Reason
{
    #region Interfaces

    /// <summary>
    /// Represents a premise used in Inductive Reasoning
    /// </summary>
    public interface IEvidence<T> : IPremise<T>
    {
        Int Strength { get; }
    }

    /// <summary>
    /// Represents a conclusion used in Inductive Reasoning
    /// </summary>
    public interface IProbability<T> : IConclusion<T>
    {
        Int Strength { get; }
    }

    /// <summary>
    /// Represents an argument where each proposition contributes to a confidence level in the conclusion.
    /// </summary>
    /// <typeparam name="TEvidence"></typeparam>
    /// <typeparam name="TProbability"></typeparam>
    /// <typeparam name="TProposition"></typeparam>
    public interface IInduction<TEvidence, TProbability, TInference, TProposition> : IArgument<Int, TEvidence, TProbability, TInference, TProposition>
                          where TEvidence : IEvidence<TEvidence>
                          where TProbability : IProbability<TProbability>
                          where TInference : IInference<TEvidence, TProbability>
                          where TProposition : IProposition<TEvidence, TInference, TProbability>
    {
        Int Cogence { get; }
    }

    #endregion

    #region Structs

    /// <summary>
    /// Represents an evidentiary premise used in Inductive Reasoning.
    /// </summary>
    [Serializable]
    public struct Evidence : IEvidence<Evidence>
    {
        public Int sememe, strength;
        public TVal Sememe<TVal>() where TVal : IValue<TVal>
        {
            if (sememe is TVal tSememe) return tSememe;
            return default;
        }
        public Int Sememe() { return sememe; }

        public Int Strength { get { return strength; } }

        public Evidence(int sememe, int strength)
        {
            this.sememe = sememe;
            this.strength = strength;
        }
        public bool Equals(Evidence obj)
        {
            return false;
        }
    }

    /// <summary>
    /// Represents a probable conclusion used in Inductive Reasoning.
    /// </summary>
    [Serializable]
    public struct ProbableConclusion : IProbability<ProbableConclusion>
    {
        public Int sememe, strength;
        public TVal Sememe<TVal>() where TVal : IValue<TVal>
        {
            if (sememe is TVal tSememe) return tSememe;
            return default;
        }
        public Int Sememe() { return sememe; }

        public Int Strength { get { return strength; } }

        public ProbableConclusion(int sememe, int strength)
        {
            this.sememe = sememe;
            this.strength = strength;
        }

        public bool Equals(ProbableConclusion obj)
        {
            return false;
        }
    }

    public struct InductiveInference : IInference<Evidence, ProbableConclusion>
    {
        public ProbableConclusion Infer(Evidence A, Evidence B)
        {
            throw new System.NotImplementedException();
        }

    }

    [Serializable]
    public struct InductiveProposition : IProposition<Evidence, InductiveInference, ProbableConclusion>
    {
        public Evidence Antecedent => throw new System.NotImplementedException();

        public InductiveInference Copula => throw new System.NotImplementedException();

        public Evidence Consequent => throw new System.NotImplementedException();

        public ProbableConclusion Conclusion => throw new System.NotImplementedException();

        public Bool IsTrue => throw new System.NotImplementedException();
    }

    [Serializable]
    public struct InductiveArgument : IInduction<Evidence, ProbableConclusion, InductiveInference, InductiveProposition>
    {
        public Int cogence, soundness;

        public Int Cogence { get { return cogence; } }

        public Int Soundness { get { return soundness; } }

        public void BuildArgument(InductiveProposition[] premises, ProbableConclusion conclusion)
        {

        }
    }

    #endregion

}
