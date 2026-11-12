using System;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{

    [Serializable]
    public struct NarremeCondition : IIncidentCondition, IEquatable<NarremeCondition>
    {
        public UnityIncident hostEvent;
        
        public int conditionSubject, condition, conditionObject;

        public bool subjectIsSymbolic, objectIsSymbolic;
        
        public bool CheckCondition()
        {

            return false;
        }

        public bool Equals(NarremeCondition other)
        {
            return this == other;
        }

        public override bool Equals(object obj)
        {
            if(obj is NarremeCondition nC)
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

        public static bool operator ==(NarremeCondition a, NarremeCondition b)
        {
            return (a.hostEvent == b.hostEvent &&
                    a.subjectIsSymbolic == b.subjectIsSymbolic &&
                    a.conditionSubject == b.conditionSubject &&
                    a.condition == b.condition &&
                    a.objectIsSymbolic == b.objectIsSymbolic &&
                    a.conditionObject == b.conditionObject);
        }
        public static bool operator != (NarremeCondition a, NarremeCondition b)
        {
            return !(a == b);
        }
    }
}
