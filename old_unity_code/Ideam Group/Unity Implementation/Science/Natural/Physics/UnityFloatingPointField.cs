using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Natural.Physics;


namespace Ideam.Unity.Science.Natural.Physics
{
    [System.Serializable]
    public class UnityFloatingPointField : IFundamentalField
    {
        public string name;

        public Float fieldConstant, infinimum, supremum;

        public Float FieldConstant() { return fieldConstant; }

        public T FieldConstant<T>() where T : IValue<T>
        {
            if (fieldConstant is T tConstant) { return tConstant; }
            return default;
        }

        public Float Infinimum() { return infinimum; }

        public T Infinimum<T>() where T : IValue<T>
        {
            if (infinimum is T tInfinimum) { return tInfinimum; }
            return default;
        }

        public Float Supremum() { return supremum; }

        public T Supremum<T>() where T : IValue<T>
        {
            if (supremum is T tSupremum) { return tSupremum; }
            return default;
        }
    }
}