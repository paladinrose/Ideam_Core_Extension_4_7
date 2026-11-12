using System.Collections.Generic;
using System.Text;

using Ideam;
using Ideam.Axiology.Valuation;
using Ideam.Epistemology.Semantics;
using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Action;

namespace Ideam.Axiology.Aesthetics.The_Arts.Narratology
{
    public enum ConditionStatus
    {
        NotMet, Met, CannotMeet
    }

    public interface INarreme 
    {
        ConditionStatus CheckNarrativeCondition<T>(int condition, T narremeToCheck = default) where T : INarreme;

        T[] GetSymbolicForms<T>() where T:INarreme;
        T GetLiteralForm<T>() where T:INarreme;
    }

    
}
