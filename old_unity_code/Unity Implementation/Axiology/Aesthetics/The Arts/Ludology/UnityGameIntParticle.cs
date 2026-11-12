using System;
using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity
{

    [Serializable]
    public struct UnityGameIntParticle: IQuanta
    {
        public int sememe;

        public bool isFundamental;
        
        public bool IsFundamental { get { return isFundamental; } }
        
        public Int force;

        public T[] ActualValues<T>() where T : IValue<T>
        {
            throw new NotImplementedException();
        }

        public T Measure<T, TField, TPointer>(TField field, TPointer pointerState)
            where T : IValue<T>
            where TField : IFundamentalField
            where TPointer : IPointerState
        {
            throw new NotImplementedException();
        }
    }

}