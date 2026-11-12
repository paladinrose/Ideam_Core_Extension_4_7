using System;
using Ideam.Axiology.Valuation;

using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Identity;

namespace Ideam.Unity
{
      
    [Serializable]
    public struct UnityGamePieceProperty : IProperty
    {
        public int sememe;
        public Int value;

        public event Action<UnityGamePieceProperty, Int> OnValueChanged;
        public int Sememe()
        {
            return sememe;
        }

        public T Value<T>() where T : IValue<T>
        {
            if (value is T tVal) { return tVal; }
            return default;
        }

        public Int CurrentValue { get { return value; } set
            {
                if(value != this.value)
                {
                    Int o = this.value;
                    this.value = value;
                    OnValueChanged?.Invoke(this, o);
                }
            } }
    }

}