using System.Collections.Generic;

using UnityEngine;
using Ideam.Metaphysics.Mind.Perception;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Information;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Axiology.Valuation;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    //Takes in signals of type T and decodes it into semantic info.
    public abstract class UnitySense<TSenseInput> : ISense
        where TSenseInput : ISignal
    {
        public UnityMind mind;

        public class SensedThing
        {
            public bool tryingToSense, hasIdentifiedThing;
            public UnityGamePiece sensedThing;
            public TSenseInput transmittedInput, senseInput;

        }

        public List<SensedThing> sensedThings = new List<SensedThing>();

        public virtual void Setup(GameObject sourceObject) 
        {

        }

        public abstract void EnableSensing();

        public abstract void DisableSensing();
        
        public virtual void BeginReception<T>(T signal) where T : ISignal
        {
            if (signal is TSenseInput senseSignal) { BeginReception(senseSignal); }
        }

        public virtual void BeginReception(TSenseInput signal) 
        {
        
        }

        public virtual void Receive<T, TValue, TSignal>(TSignal signal, T deltaTime)
            where T : IValue<TValue>
            where TSignal : ISignal
        {
            if(signal is TSenseInput senseSignal && deltaTime is Float dt)
            {
                Receive(senseSignal, dt);
            }
        }

        public virtual void Receive(TSenseInput signal, Float deltaTime)
        {

        }

        public virtual TDecoded DecodeSignal<TSignal, TDecoded>(TSignal signalToDecode) where TSignal : ISignal
        {
            if (signalToDecode is TSenseInput senseSignal)
            {
                int[] signalSemantics = DecodeSignal(senseSignal);
                if (signalSemantics is TDecoded decodeSemantics)
                {
                    return decodeSemantics;
                }
            }

            return default;
        }

        public virtual int[] DecodeSignal(TSenseInput signalToDecode)
        {
            return new int[0];
        }

        public virtual T GetSignal<T>(int id) where T : ISignal
        {
            if (id < 0 || id >= sensedThings.Count) { return default; }

            if (sensedThings[id].transmittedInput is T tInput) { return tInput; }

            return default;
        }

        public virtual TSenseInput GetSignal(int id)
        {
            if (id < 0 || id >= sensedThings.Count) { return default; }
            return sensedThings[id].transmittedInput;

        }

        public virtual T[] GetSignalsReceiving<T>() where T : ISignal
        {
            List<T> tSigs = new List<T>();
            for (int i = 0; i < sensedThings.Count; i++)
            {
                if (sensedThings[i] is T tSig) { tSigs.Add(tSig); }
            }
            return tSigs.ToArray();
        }

        public virtual bool IsAttemptingToReceiveSignal(int id)
        {
            if (id < 0 || id >= sensedThings.Count) { return false; }
            return sensedThings[id].tryingToSense;
        }

        public virtual int IsReceivingID<T>(T signal) where T : ISignal
        {
            if (signal is TSenseInput senseSignal)
            {
                for (int i = 0; i < sensedThings.Count; i++)
                {
                    if (sensedThings[i].senseInput.Equals(senseSignal)) { return i; }
                }
            }
            return -1;
        }

        public virtual void EndReception<T>(T signal) where T : ISignal
        {
            if(signal is TSenseInput senseSignal) { EndReception(senseSignal); }
        }

        public virtual void EndReception(TSenseInput signal)
        {

        }

        public virtual void BeginSensing<T>(T sensedThing) where T : IThing 
        {
            if(sensedThing is UnityGamePiece sensedPiece) { BeginSensing(sensedPiece); }
        }
        public virtual void BeginSensing(UnityGamePiece sensedThing) 
        {
        }
        public virtual void Sensing<T>(T sensedThing) where T : IThing
        {
            if(sensedThing is UnityGamePiece sensedPiece) { Sensing(sensedPiece); }
        }
        public virtual void Sensing(UnityGamePiece sensedThing) 
        {
        }
        public virtual void StopSensing<T>(T notSensedThing) where T : IThing
        {
            if(notSensedThing is UnityGamePiece notSensedPiece) { StopSensing(notSensedPiece); }
        }
        public virtual void StopSensing(UnityGamePiece notSensedThing) { }

        public virtual void NewInformation(List<Information> newInfoList) 
        {
            
        }
    }
}