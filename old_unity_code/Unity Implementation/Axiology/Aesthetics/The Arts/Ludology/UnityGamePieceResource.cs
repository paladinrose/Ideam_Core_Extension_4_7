using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Existence;
using System;

using Ideam.Metaphysics.Ontology.Identity;
using Ideam.Axiology.Valuation;

namespace Ideam.Unity
{
    [Serializable]
    public struct UnityGamePieceResource : IResource
    {
        public int sememe;
        public Int value, maxValue, exhaustionPoint;
        public event Action<UnityGamePieceResource> OnUse, OnCannotUse,  OnExhausted;
        public event Action<UnityGamePieceResource, Int> OnValueChanged, OnMaxValueChanged;
        public T MaximumValue<T>() where T : IValue<T>
        {
            if(maxValue is T tMax) { return tMax; }
            return default;
        }

        public int Sememe()
        {
            return sememe;
        }

        public Int UseResource()
        {
            if(value == maxValue && value == exhaustionPoint) { OnUse?.Invoke(this); return value; }

            if(value <= exhaustionPoint)
            { OnCannotUse?.Invoke(this); value -= 1; return value; }

            Int v = value;
            value -= 1;
            OnUse?.Invoke(this);

            if(value <= exhaustionPoint) { OnExhausted?.Invoke(this); }
            return v;
        }

        public T UseResource<T>() where T : IValue<T>
        {
            
            Int newVal = value - 1;
            
            if(value is T tVal)
            {
                value = newVal;
                return tVal;
            }

            return default;
        }

        public T Value<T>() where T : IValue<T>
        {
            if (value is T tVal) { return tVal; }
            return default;
        }

        public Int CurrentValue { get { return value; } 
            set 
            {
                Int v = this.value;
                this.value = value;
                if(value < exhaustionPoint) { OnExhausted?.Invoke(this); return; }
                OnValueChanged?.Invoke(this, v);
            } 
        }
        public Int MaxValue { get { return maxValue; } set { Int mv = maxValue; maxValue = value; OnMaxValueChanged?.Invoke(this, mv); } }
    }
}