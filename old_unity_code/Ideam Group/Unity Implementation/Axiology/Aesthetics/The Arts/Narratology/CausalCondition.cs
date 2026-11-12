using System;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    [Serializable]
    public class CausalCondition : IIncidentCondition, IEquatable<CausalCondition>
    {
        public UnityIncident hostEvent;
        
        public int[] causes = new int[0];

        public bool[] hasCaused = new bool[0], 
                      causeIsSymbolic = new bool[0];

        public bool CheckCondition()
        {
            if(hasCaused.Length == 0) { return false; }
            for(int i = hasCaused.Length-1; i >=0; i--)
            {
                if (!hasCaused[i]) { return false; }
            }
            return true;
        }

        public bool Equals(CausalCondition other)
        {
            return this == other;
        }

        public override bool Equals(object obj)
        {
            if (obj is CausalCondition nC)
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

        public static bool operator ==(CausalCondition a, CausalCondition b)
        {
            if(a.hostEvent != b.hostEvent) { return false; }
            if(a.causes.Length != b.causes.Length) { return false; }
            for(int i = a.causes.Length-1; i >=0; i--)
            {
                if(a.causes[i] != b.causes[i] || a.causeIsSymbolic[i] != b.causeIsSymbolic[i]) { return false; }
            }
            return true;
        }

        public static bool operator !=(CausalCondition a, CausalCondition b)
        {
            return !(a == b);
        }
    }
}
