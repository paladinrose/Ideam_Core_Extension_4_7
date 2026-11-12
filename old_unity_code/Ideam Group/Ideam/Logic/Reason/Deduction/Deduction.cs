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
    /// Represents a premise used in Deductive Reasoning
    /// </summary>
    public interface IStatement<T> : IPremise<T>
    {
        Bool Sound { get; }
    }

    /// <summary>
    /// Represents a conclusion used in Deductive Reasoning
    /// </summary>
    public interface IConsequence<T> : IConclusion<T>
    {
        Bool Sound { get; }
    }

    public interface ICategoricalStatement<T> : IStatement<T>
    {
        Quantifier Quantifier { get; }
    }

    public interface ICategoricalConsequence<T> : IConsequence<T>
    {
        Quantifier Quantifier { get; }
    }

    /// <summary>
    /// Represents an argument where, if the propositions presented are true, the conclusion
    /// must also logically be true.
    /// </summary>
    /// <typeparam name="TStatement"></typeparam>
    /// <typeparam name="TConsequence"></typeparam>
    /// <typeparam name="TProposition"></typeparam>
    public interface IDeduction<TStatement, TConsequence, TInference, TProposition> : IArgument<Bool, TStatement, TConsequence, TInference, TProposition>
                          where TStatement : IStatement<TStatement>
                          where TConsequence : IConsequence<TConsequence>
                          where TInference : IInference<TStatement, TConsequence>
                          where TProposition : IProposition<TStatement, TInference, TConsequence>
    {
        Bool Validity { get; }
    }

    #endregion

    #region Structs

    /// <summary>
    /// Represents a stated premise in Deductive Reasoning.
    /// </summary>
    [Serializable]
    public struct Statement : IStatement<Statement>
    {
        public Int sememe;

        public TVal Sememe<TVal>() where TVal : IValue<TVal>
        {
            if (sememe is TVal tSememe) return tSememe;
            return default;
        }
        public Int Sememe() { return sememe; }

        public Bool Sound { get; private set; }

        public Statement(int sememe, bool sound)
        {
            this.sememe = sememe;
            Sound = sound;
        }

        public bool Equals(Statement obj)
        {
            return Sememe() == obj.Sememe() && Sound == obj.Sound;
        }
    }

    /// <summary>
    /// Represents a consequential conclusion in Deductive Reasoning.
    /// </summary>
    [Serializable]
    public struct Consequence : IConsequence<Consequence>
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

        public Consequence(int sememe, int truth)
        {
            this.sememe = sememe;
            this.truth = truth;
        }

        public bool Equals(Consequence obj)
        {
            return (this.sememe == obj.sememe && this.truth == obj.truth);
        }
    }


    /// <summary>
    /// Represents a categorically stated premise in Deductive Reasoning.
    /// </summary>
    [Serializable]
    public struct CategoricalStatement : ICategoricalStatement<CategoricalStatement>
    {
        public Int sememe;
        public TVal Sememe<TVal>() where TVal : IValue<TVal>
        {
            if (sememe is TVal tSememe) return tSememe;
            return default;
        }
        public Int Sememe() { return sememe; }
        public Bool Sound { get; private set; }
        public Quantifier Quantifier { get; private set; }

        public CategoricalStatement(int sememe, bool sound, Quantifier quantifier)
        {
            this.sememe = sememe;
            Sound = sound;
            Quantifier = quantifier;
        }

        public bool Equals(CategoricalStatement obj)
        {
            return (this.sememe == obj.sememe && this.Sound == obj.Sound && this.Quantifier == obj.Quantifier);
        }
    }

    /// <summary>
    /// Represents a categorically consequential conclusion in Deductive Reasoning.
    /// </summary>
    [Serializable]
    public struct CategoricalConsequence : ICategoricalConsequence<CategoricalConsequence>
    {
        public Int sememe;
        public TVal Sememe<TVal>() where TVal : IValue<TVal>
        {
            if (sememe is TVal tSememe) return tSememe;
            return default;
        }
        public Int Sememe() { return sememe; }

        public Bool sound;
        Quantifier quantifier;

        public Bool Sound { get { return sound; } }
        public Quantifier Quantifier { get { return quantifier; } }

        public CategoricalConsequence(int sememe, bool sound, Quantifier quantifier)
        {
            this.sememe = sememe;
            this.sound = sound;
            this.quantifier = quantifier;
        }

        public bool Equals(CategoricalConsequence obj)
        {
            return (this.sememe == obj.sememe && this.sound == obj.sound && this.quantifier == obj.quantifier);
        }
    }


    /// <summary>
    /// Takes in a collection of fact. Returns a fact that separately specifies whether the supplied
    /// facts context and truth fields are the same.
    /// </summary>
    public struct TransitiveInference : IInference<Fact, Fact>
    {
        public static Int TrueSememe, FalseSememe;
        public Fact Infer(Fact A, Fact B)
        {
            Int con, tru;

            if (A.Sememe() == B.Sememe()) { con = TrueSememe; } else { con = FalseSememe; }
            if (A.Truth == B.Truth) { tru = TrueSememe; } else { tru = FalseSememe; }

            Fact inf = new Fact(con, tru);
            return inf;
        }

    }

    [Serializable]
    public struct ConditionalProposition : IProposition<Fact, TransitiveInference, Fact>
    {
        [SerializeField]
        Fact antecedent, consequent, conclusion;
        public Fact Antecedent { get { return antecedent; } }

        public TransitiveInference Copula { get { return new TransitiveInference(); } }

        public Fact Consequent { get { return consequent; } }

        public Fact Conclusion { get { return conclusion; } }
        public Bool IsTrue
        {
            get
            {
                conclusion = Copula.Infer(antecedent, consequent);
                return conclusion.Sememe() == TransitiveInference.TrueSememe;
            }
        }


    }


    public struct SyllogisticInference : IInference<CategoricalStatement, CategoricalConsequence>
    {
        public CategoricalConsequence Infer(CategoricalStatement A, CategoricalStatement B)
        {
            throw new System.NotImplementedException();
        }
    }

    [Serializable]
    public struct SyllogisticProposition : IProposition<CategoricalStatement, SyllogisticInference, CategoricalConsequence>
    {
        [SerializeField]
        CategoricalStatement antecedent, consequent;

        [SerializeField]
        CategoricalConsequence concers;

        [SerializeField]
        SyllogisticFigure figure;

        public CategoricalStatement Antecedent { get { return antecedent; } }

        public SyllogisticInference Copula { get { return new SyllogisticInference(); } }

        public CategoricalStatement Consequent { get { return consequent; } }

        public CategoricalConsequence Conclusion { get { return concers; } }

        public SyllogisticFigure Figure { get { return figure; } }

        public Bool IsTrue
        {
            get
            {
                concers = Copula.Infer(antecedent, consequent);
                return concers.Sound;
            }
        }

        public bool IsValid
        {
            get
            {
                Quantifier majorPremise = antecedent.Quantifier,
                           minorPremise = consequent.Quantifier,
                           conclusion = concers.Quantifier;

                switch (figure)
                {
                    case SyllogisticFigure.MP_SM:
                        if (majorPremise == Quantifier.All)
                        {
                            if (minorPremise == Quantifier.All)
                            {
                                if (conclusion == Quantifier.All ||
                                    conclusion == Quantifier.Some)
                                { return true; }
                            }

                            if (minorPremise == Quantifier.Some &&
                                conclusion == Quantifier.Some)
                            { return true; }
                        }
                        else if (majorPremise == Quantifier.None)
                        {
                            if (minorPremise == Quantifier.All)
                            {
                                if (conclusion == Quantifier.None ||
                                    conclusion == Quantifier.NotSome)
                                { return true; }
                            }
                            else if (minorPremise == Quantifier.Some &&
                                      conclusion == Quantifier.NotSome)
                            { return true; }
                        }
                        break;

                    case SyllogisticFigure.PM_SM:
                        if (majorPremise == Quantifier.All)
                        {
                            if (minorPremise == Quantifier.None)
                            {
                                if (conclusion == Quantifier.None ||
                                    conclusion == Quantifier.NotSome)
                                { return true; }
                            }
                            else if (minorPremise == Quantifier.NotSome &&
                                       conclusion == Quantifier.NotSome)
                            { return true; }
                        }
                        else if (majorPremise == Quantifier.None)
                        {
                            if (minorPremise == Quantifier.All)
                            {
                                if (conclusion == Quantifier.None ||
                                    conclusion == Quantifier.NotSome)
                                { return true; }
                            }
                            else if (minorPremise == Quantifier.Some &&
                                     conclusion == Quantifier.NotSome)
                            { return true; }
                        }
                        break;

                    case SyllogisticFigure.MP_MS:
                        if (majorPremise == Quantifier.All)
                        {
                            if (conclusion == Quantifier.Some)
                            {
                                if (minorPremise == Quantifier.All ||
                                    minorPremise == Quantifier.Some) { return true; }
                            }
                        }
                        else if (majorPremise == Quantifier.None)
                        {
                            if (conclusion == Quantifier.NotSome)
                            {
                                if (minorPremise == Quantifier.All ||
                                    minorPremise == Quantifier.Some)
                                { return true; }
                            }
                        }
                        else if (majorPremise == Quantifier.Some &&
                                  minorPremise == Quantifier.All &&
                                  conclusion == Quantifier.Some)
                        { return true; }
                        else if (majorPremise == Quantifier.NotSome &&
                            minorPremise == Quantifier.All &&
                            conclusion == Quantifier.NotSome)
                        { return true; }
                        break;

                    case SyllogisticFigure.PM_MS:
                        if (majorPremise == Quantifier.All)
                        {
                            if (minorPremise == Quantifier.None)
                            {
                                if (conclusion == Quantifier.None ||
                                    conclusion == Quantifier.NotSome)
                                { return true; }
                            }
                            else if (minorPremise == Quantifier.All &&
                                     conclusion == Quantifier.Some)
                            { return true; }
                        }
                        else if (majorPremise == Quantifier.None)
                        {
                            if (conclusion == Quantifier.NotSome)
                            {
                                if (minorPremise == Quantifier.Some ||
                                    minorPremise == Quantifier.All)
                                { return true; }
                            }
                        }
                        else if (majorPremise == Quantifier.Some &&
                            minorPremise == Quantifier.All &&
                            conclusion == Quantifier.Some)
                        { return true; }
                        break;
                }
                return false;
            }
        }

    }

    [Serializable]
    public struct Syllogism : IDeduction<CategoricalStatement, CategoricalConsequence, SyllogisticInference, SyllogisticProposition>
    {
        public Bool valid, sound;

        public Bool Validity { get { return valid; } }

        public Bool Soundness { get { return sound; } }

        public void BuildArgument(SyllogisticProposition[] premises, CategoricalConsequence conclusion)
        {
            valid = sound = true;
            for (int i = 0; i < premises.Length; i++)
            {
                if (!premises[i].IsTrue) { sound = false; }
                if (!premises[i].IsValid) { valid = false; }
                if (!sound && !valid) { return; }

                CategoricalConsequence consequence = premises[i].Conclusion;
                if (consequence.Sememe() != conclusion.Sememe()) { sound = false; }
            }
        }
    }

    #endregion


}
