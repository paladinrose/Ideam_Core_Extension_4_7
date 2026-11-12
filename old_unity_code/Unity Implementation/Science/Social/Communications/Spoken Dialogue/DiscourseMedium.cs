using System;
using UnityEngine;
using Ideam.Science.Formal.Mathematics.Information;

namespace Ideam.Unity.Science.Social.Communications
{
    public class DiscourseMedium : MonoBehaviour, IMedium
    {
        public T GetNoise<T, TSignal>(TSignal signal)
            where T : INoise
            where TSignal : ISignal
        {
            throw new NotImplementedException();
        }

        public T[] GetReceivers<T>() where T : IReceiver
        {
            throw new NotImplementedException();
        }

        public T[] GetSignalsInTransmission<T>() where T : ISignal
        {
            throw new NotImplementedException();
        }

        public T[] GetTransmitters<T>() where T : ITransmitter
        {
            throw new NotImplementedException();
        }
    }
}