using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;

namespace Ideam.Science.Formal.Mathematics.Information
{
    public interface IReceiver
    {
        void BeginReception<T>(T signal) where T : ISignal;
        void Receive<T, TValue, TSignal>(TSignal signal, T deltaTime) where T : IValue<TValue> where TSignal : ISignal;
        
        T[] GetSignalsReceiving<T>() where T : ISignal;
        
        T GetSignal<T>(int id) where T : ISignal;
        int IsReceivingID<T>(T signal) where T : ISignal;
        bool IsAttemptingToReceiveSignal(int id);
        void EndReception<T>(T signal) where T : ISignal;
    }
}
