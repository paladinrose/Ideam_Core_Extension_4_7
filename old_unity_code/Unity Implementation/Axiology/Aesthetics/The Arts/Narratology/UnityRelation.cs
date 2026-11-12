using System;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;
using Ideam.Axiology.Valuation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    
    [Serializable]
    public class UnityRelation : IntAffinity, IRelation<UnityNarreme> 
    {
        public string title;
        public UnityNarreme subject;
        public UnityNarreme Subject { get { return subject; } }

        public Tn GetSubject<Tn>() where Tn : INarreme
        {
            if(subject is Tn tSub) { return tSub; }

            return default;
        }
    }

}
