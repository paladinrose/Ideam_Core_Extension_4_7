using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Systems;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Science.Formal.Mathematics.Information.Cryptography;

namespace Ideam.Science.Natural.Cosmology
{
    public class SpacetimeSystem : IDynamicSystem
    {
        public T GetEnvironment<T>() where T : IPlace
        {
            throw new System.NotImplementedException();
        }

        public T GetUser<T>(int id) where T : IUser
        {
            throw new System.NotImplementedException();
        }

        public T[] GetUsers<T>() where T : IUser
        {
            throw new System.NotImplementedException();
        }

        public T Security<T>() where T : ISecurity
        {
            throw new System.NotImplementedException();
        }
    }
}
