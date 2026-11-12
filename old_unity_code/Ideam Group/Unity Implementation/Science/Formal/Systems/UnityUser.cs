using System.Collections;
using System.Collections.Generic;
using UnityEngine;



using Ideam.Science.Formal.Systems;
using Ideam.Science.Formal.Mathematics.Information.Cryptography;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Science.Formal.Systems
{
    public class UnityUser : MonoBehaviour, IUser
    {
        public UnityGamePlayer Player { get; }
        public T SecurityPass<T>() where T : ISecurityPass
        {
            throw new System.NotImplementedException();
        }
    }
}