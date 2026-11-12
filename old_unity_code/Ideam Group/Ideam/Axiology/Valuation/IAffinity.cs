using System;
using System.Text;

using Ideam.Epistemology.Semantics;

namespace Ideam.Axiology.Valuation
{
    /// <summary>
    /// Extends Semantic objects by attaching a value based Affinity to each Sememe.
    /// </summary>
    public interface IAffinity : ISemantic
    {
        int AddSemanticAffinity<T>(int globalSememeID, T affinity);
        T[] GetSemanticAffinities<T>() where T:IValue<T>;
        T GetSememeAffinity<T>(int localID) where T : IValue<T>;
        void SetSememeAffinity<T>(int localID, T newAffinity) where T : IValue<T>;
        bool CompareSemanticAffinity<T>(IAffinity other, out T degreeOfDifference) where T : IValue<T>;
    }
}
