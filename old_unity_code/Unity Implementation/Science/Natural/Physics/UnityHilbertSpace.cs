using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Natural.Physics.QuantumMechanics;
using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

using Ideam.Science.Natural.Cosmology;

namespace Ideam.Unity.Science.Natural.Physics
{
    public class UnityHilbertSpace : MonoBehaviour, IHilbertSpace
    {
        public T GetFieldLimit<T>(int id) where T : IValue<T>
        {
            throw new System.NotImplementedException();
        }

        public T Sample<T, TID>(TID location, int fieldID)
            where T : IQuanta
            where TID : IValue<TID>
        {
            throw new System.NotImplementedException();
        }

        public T Universe<T>() where T : IUniverse
        {
            throw new System.NotImplementedException();
        }
        public virtual T Bounds<T, TValue>() where T : IBoundary<T, TValue> where TValue : IValue<TValue>
        {
            return default;
        }
    }
}