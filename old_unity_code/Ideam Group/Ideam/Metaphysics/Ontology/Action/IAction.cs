using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;

namespace Ideam.Metaphysics.Ontology.Action
{
    public interface IAction
    {
        GamePieceUseData[] Sequence { get; }
        ActionType ActionType { get; }
    }

    public enum ActionType
    {
        Singular, Continuous, Repeated
    }

    [System.Serializable]
    public struct GamePieceUseData
    {
        public int pieceID;
        public int propertyID;
        public int resourceID;
        //-1 = simultaneous with next
        //0 = end of simultaneous.
        //
        public int sequencing, sequenceGoTo;
        public float sequenceTime;
        public bool updateActionValue;
    }
}
