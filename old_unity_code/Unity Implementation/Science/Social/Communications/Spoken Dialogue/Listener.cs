using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Epistemology.Semantics;

using Ideam.Science.Formal.Linguistics;
using Ideam.Science.Formal.Graphs;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Information;
using Ideam.Axiology.Valuation;

using Ideam.Unity.Metaphysics.Mind;
namespace Ideam.Unity.Science.Social.Communications
{
    public class Listener : MonoBehaviour, IDecoder, IReceiver
    {
        public UnityMind mind;

        #region Reception
        public void BeginReception<T>(T signal) where T : ISignal
        {
            throw new NotImplementedException();
        }

        public void Receive<T, TValue, TSignal>(TSignal signal, T deltaTime)
           where T : IValue<TValue>
           where TSignal : ISignal
        {
            throw new NotImplementedException();
        }

        public void EndReception<T>(T signal) where T : ISignal
        {
            throw new NotImplementedException();
        }

        public T GetSignal<T>(int id) where T : ISignal
        {
            throw new NotImplementedException();
        }

        public T[] GetSignalsReceiving<T>() where T : ISignal
        {
            throw new NotImplementedException();
        }

        public bool IsAttemptingToReceiveSignal(int id)
        {
            throw new NotImplementedException();
        }

        public int IsReceivingID<T>(T signal) where T : ISignal
        {
            throw new NotImplementedException();
        }

       
        #endregion

        #region Decoder
        public TDecoded DecodeSignal<TSignal, TDecoded>(TSignal signalToDecode) where TSignal : ISignal
        {
            throw new NotImplementedException();
        }

        #endregion
    }
}
