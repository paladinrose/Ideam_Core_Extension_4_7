using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Science.Social.Communications;

using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    public class DetectableVisual : UnityTransmitter<UnityVisualSignal>
    {
        public RaySensorEvent OnSeen, WhileSeen, OnStoppedBeingSeen, OnNotSeen;

        public override event Action<UnityVisualSignal> OnBeginTransmitting;
        public override event Action<UnityVisualSignal> WhileTransmitting;
        public override event Action<UnityVisualSignal> OnStopTransmitting;

        public override void BeginTransmission(UnityVisualSignal signal)
        {
            if (currentSignals.Contains(signal)) { return; }

            currentSignals.Add(signal);

            OnBeginTransmitting?.Invoke(signal);
        }

        public override void EndTransmission(UnityVisualSignal signal)
        {
            if (!currentSignals.Contains(signal)) { return; }

            currentSignals.Remove(signal);

            OnStopTransmitting?.Invoke(signal);
        }

        public override UnityVisualSignal Transmit(UnityVisualSignal signal, Float deltaTime)
        {

            WhileTransmitting?.Invoke(signal);
            return signal;

        }
    }
}