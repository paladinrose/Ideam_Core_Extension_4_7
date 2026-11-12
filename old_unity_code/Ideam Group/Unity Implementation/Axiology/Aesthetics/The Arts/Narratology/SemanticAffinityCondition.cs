using System;

using Ideam.Axiology.Valuation;
using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{

    [Serializable]
    public struct SemanticAffinityCondition : IIncidentCondition, IEquatable<SemanticAffinityCondition>
    {
        public UnityIncident hostEvent;
        public int conditionSubject, subjectSememe, conditionObject, objectSememe;
        public bool subjectIsSymbolic, objectIsSymbolic;

        public ValueComparison affinityComparison;

        public bool CheckCondition()
        {
            return false;
        }

        public bool Equals(SemanticAffinityCondition other)
        {
            return this == other;
        }

        public override bool Equals(object obj)
        {
            if (obj is SemanticAffinityCondition nC)
            {
                return this == nC;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public override string ToString()
        {
            return base.ToString();
        }

        public static bool operator ==(SemanticAffinityCondition a, SemanticAffinityCondition b)
        {
            return (a.hostEvent == b.hostEvent &&
                    a.subjectIsSymbolic == b.subjectIsSymbolic &&
                    a.conditionSubject == b.conditionSubject &&
                    a.subjectSememe == b.subjectSememe &&
                    a.objectIsSymbolic == b.objectIsSymbolic &&
                    a.conditionObject == b.conditionObject &&
                    a.objectSememe == b.objectSememe);
        }

        public static bool operator !=(SemanticAffinityCondition a, SemanticAffinityCondition b)
        {
            return !(a == b);
        }
    }
}
