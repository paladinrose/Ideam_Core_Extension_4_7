using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Information;
using Ideam.Axiology.Valuation;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Science.Social.Communications
{
    public abstract class UnityTransmitter<TSignal> : UnityGamePiece, ITransmitter
        where TSignal : ISignal
    {

        public List<TSignal> currentSignals = new List<TSignal>();

        protected TSignal lastSignalTransmitted;

        public abstract event Action<TSignal> OnBeginTransmitting, WhileTransmitting, OnStopTransmitting;

        public virtual void BeginTransmission<T>(T signal) where T : ISignal
        {
            if (signal is TSignal tSig) { BeginTransmission(tSig); }
        }

        public abstract void BeginTransmission(TSignal signal);

        public virtual void EndTransmission<T>(T signal) where T : ISignal
        {
            if (signal is TSignal tSig) { EndTransmission(tSig); }
        }

        public abstract void EndTransmission(TSignal signal);

        public virtual T GetSignal<T>(int id) where T : ISignal
        {
            if(id <0 || id >= currentSignals.Count) { return default; }

            if(currentSignals[id] is T tSignal) { return tSignal; }
            return default;
        }

        public virtual T[] GetSignalsTransmitting<T>() where T : ISignal
        {
            List<T> tSignals = new List<T>();
            for(int i =0; i < currentSignals.Count; i++)
            {
                if(currentSignals[i] is T tSignal) { tSignals.Add(tSignal); }
            }
            return tSignals.ToArray();
        }

        public virtual void Transmit<T, TValue, TSignal1>(TSignal1 signal, T deltaTime)
            where T : IValue<TValue>
            where TSignal1 : ISignal
        {
            if(signal is TSignal tSignal)
            {
                if(deltaTime is Float dt)
                {
                   lastSignalTransmitted = Transmit(tSignal, dt);
                }
            }
        }

        public abstract TSignal Transmit(TSignal signal, Float deltaTime);
    }
}