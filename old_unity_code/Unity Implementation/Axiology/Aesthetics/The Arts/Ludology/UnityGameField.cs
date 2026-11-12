using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

namespace Ideam.Unity
{
    [Serializable]
    public class UnityGameField : IFundamentalField
    {
        public T FieldConstant<T>() where T : IValue<T>
        {
            throw new NotImplementedException();
        }

        public T Infinimum<T>() where T : IValue<T>
        {
            throw new NotImplementedException();
        }

        public T Supremum<T>() where T : IValue<T>
        {
            throw new NotImplementedException();
        }
    }

}