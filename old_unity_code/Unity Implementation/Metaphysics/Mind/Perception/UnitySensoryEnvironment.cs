using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Science.Formal.Mathematics.Information;
namespace Ideam.Unity
{
    public abstract class UnitySensoryEnvironment<TSenseSignal, TSenseNoise, TTransmitter, TReceiver> : MonoBehaviour, IMedium
        where TSenseSignal : ISignal
        where TSenseNoise : INoise
        where TTransmitter : ITransmitter
        where TReceiver : IReceiver
    {
        [SerializeField]
        protected bool isGlobal;

        public virtual bool IsGlobal { get { return isGlobal; } set { SetIsGlobal(value); } }

        protected virtual void SetIsGlobal(bool ig)
        {
            isGlobal = ig;
        }

        public List<TSenseSignal> currentSignals;
        public List<TTransmitter> currentTransmitters;
        public List<TReceiver> currentReceivers;

        public virtual T GetNoise<T, TSignal>(TSignal signal)
            where T : INoise
            where TSignal : ISignal
        {
            if(signal is TSenseSignal senseSignal)
            {
                TSenseNoise senseNoise = GetNoise(senseSignal);
                if(senseNoise is T noise) { return noise; }
            }
            return default;
        }
        public virtual TSenseNoise GetNoise(TSenseSignal signal)
        {
            return default;
        }

        public virtual T[] GetReceivers<T>() where T : IReceiver
        {
            List<T> tR = new List<T>();
            for(int i =0; i < currentReceivers.Count; i++)
            {
                if(currentReceivers[i] is T tRec) { tR.Add(tRec); }
            }
            return tR.ToArray();
        }

        public T[] GetSignalsInTransmission<T>() where T : ISignal
        {
            List<T> tSig = new List<T>();
            for(int i=0; i < currentSignals.Count; i++)
            {
                if(currentSignals[i] is T tS) { tSig.Add(tS); }
            }
            return tSig.ToArray();
        }
        
        public T[] GetTransmitters<T>() where T : ITransmitter
        {
            List<T> tT = new List<T>();
            for(int i =0; i < currentTransmitters.Count; i++)
            {
                if(currentTransmitters[i] is T tTransmitter) { tT.Add(tTransmitter); }
            }
            return tT.ToArray();
        }
    }
}