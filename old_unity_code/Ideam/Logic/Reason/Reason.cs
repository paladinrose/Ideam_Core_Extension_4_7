using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;
using UnityEditor;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Information;

#pragma warning disable 0649

namespace Ideam.Reason
{
    /*
     * Within logic, semantics play a huge role. Within my framework, we're using the Context system
     * as our semantic system. Thus, almost all of these are set up to use ints to represent either local
     * or global context. 
     */

    #region Delegates

    public delegate void ObservationHandler(Observation ob);

    #endregion

    #region Enums

    /// <summary>
    /// Used in propositions, particularly syllogisms, to represent the scope of a statement.
    /// </summary>
    public enum Quantifier { All, None, Some, NotSome }

    /// <summary>
    /// Represents the possible figures of an aristotelian syllogism
    /// </summary>
    public enum SyllogisticFigure { MP_SM, PM_SM, MP_MS, PM_MS }

    #endregion

    #region Interfaces
    
    /// <summary>
    /// Represents a semantic premise in a logical argument.
    /// <para>It must provide at least one seme.</para>
    /// </summary>
    public interface IPremise<T> : IEquatable<T> 
    {
        TVal Sememe<TVal>() where TVal : IValue<TVal>;
    }

    
    /// <summary>
    /// Represents a semantic conclusion based on one or more premises. 
    /// <para>It must provide at least one piece of context.</para>
    /// </summary>
    public interface IConclusion <T> : IEquatable<T> 
    {
        TVal Sememe<TVal>() where TVal : IValue<TVal>;
    }


    /// <summary>
    /// Represents a logical Rule of Inference.
    /// <para>It must define a rule that will Infer one or more <typeparamref name="TConclusion"/>s based on a set of premises</para>
    /// </summary>
    /// <typeparam name="TConclusion"></typeparam>
    public interface IInference<TPremise, TConclusion>
                            where TPremise : IPremise<TPremise>
                            where TConclusion : IConclusion<TConclusion>
    {
        TConclusion Infer(TPremise A, TPremise B);
    }


    /// <summary>
    /// Represents a logical proposition.
    /// <para>It must provide an Antecedent in the form of a <typeparamref name="TPremise"/></para>
    /// <para>It must provide a Consequent in the form of a <typeparamref name="TConclusion"/></para>
    /// <para>It must provide a true/false test to use when evaluating the proposition</para>
    /// </summary>
    /// <typeparam name="TPremise">A semantice premise to act as the Antecedent of this proposition</typeparam>
    /// <typeparam name="TConclusion">A semantice conclusion to act as the Consequent of this proposition</typeparam>
    public interface IProposition<TPremise, TInference, TConclusion>
                            where TPremise : IPremise<TPremise>
                            where TInference : IInference<TPremise, TConclusion>
                            where TConclusion : IConclusion<TConclusion>
    {
        TPremise Antecedent { get; }
        TInference Copula { get; }
        TPremise Consequent { get; }
        TConclusion Conclusion { get; }
        Bool IsTrue { get; }
    }


    /// <summary>
    /// Represents a structured argument. 
    /// <para>It must provide one or more premises and a conclusion that can be evaluated into a <typeparamref name="TruthValue"/></para>
    /// <para>It must declare whether or not it is a logically valid argument.</para>
    /// </summary>
    /// <typeparam name="TruthValue">The form that the truth of this argument will take.</typeparam>
    public interface IArgument<TruthValue, TPremise, TConclusion, TInference, TProposition>
    where TruthValue :IValue<TruthValue>
    where TPremise : IPremise<TPremise>
    where TConclusion : IConclusion<TConclusion>
    where TInference : IInference<TPremise, TConclusion>
    where TProposition : IProposition<TPremise, TInference, TConclusion>
    {
        TruthValue Soundness { get; }
        void BuildArgument(TProposition[] premises, TConclusion conclusion);
    }

    #endregion

    #region Structs

    /// <summary>
    /// Represents both statements and consequences in Deductive Reasoning. 
    /// <para>Also used for any statement with an objective truth state.</para>
    /// </summary>
    [Serializable]
    public struct Fact : IStatement<Fact>, IConsequence<Fact>
    {
        public Int sememe, truth;

        public TVal Sememe<TVal>() where TVal : IValue<TVal>
        {
            if (sememe is TVal tSememe) return tSememe;
            return default;
        }
        public Int Sememe() { return sememe; }

        public Int Truth { get { return truth; } }
        public Bool Sound { get { return truth == sememe; } }

        public Fact(int sememe, int truth)
        {
            this.sememe = sememe;
            this.truth = truth;
        }

        public bool Equals(Fact fact)
        {
            return (sememe == fact.sememe && truth == fact.truth);
        }

        
    }

    /// <summary>
    /// Represents a confluence of deductive, inductive, and abductive reasoning.
    /// </summary>
    [Serializable]
    public struct Belief : IInfo
    {
        //This should be elsewhere, to allow for different systems to have different knowledge thresholds.
        public static int KnowledgeThreshold;

        public Fact fact;

        public Int Confidence;
        public Int Acceptance;

        public void ApplyEvidence(InductiveArgument evidence)
        {

        }
        public void ApplyObservation(AbductiveArgument observation)
        {

        }
        
        public bool IsKnowledge { get { return fact.Sound && (Confidence + Acceptance) >= KnowledgeThreshold; } }

        public T GetInformation<T>()
        {
            if (fact is T tFact) { return tFact; }
            return default;
        }
    }

    #endregion

    #region Classes

    
    /// <summary>
    /// A semantic representation that uses a collection of facts and statements to represent
    /// a set of causes and their resultant consequences.
    /// </summary>
    [Serializable]
    public class Explanation
    {
        public Fact[] facts;
        public Statement[] causes;
        public Consequence[] consequences;
    }

    #endregion

}