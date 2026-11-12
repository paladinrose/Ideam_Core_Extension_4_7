using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Science.Social.Communications;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    public class DetectableHaptics : UnityTransmitter<UnityHapticSignal>
    {
        public override event Action<UnityHapticSignal> OnBeginTransmitting;
        public override event Action<UnityHapticSignal> WhileTransmitting;
        public override event Action<UnityHapticSignal> OnStopTransmitting;

        public override void BeginTransmission(UnityHapticSignal signal)
        {
            if (currentSignals.Contains(signal)) { return; }

            currentSignals.Add(signal);

            OnBeginTransmitting?.Invoke(signal);
        }

        public override void EndTransmission(UnityHapticSignal signal)
        {
            if (!currentSignals.Contains(signal)) { return; }

            currentSignals.Remove(signal);

            OnStopTransmitting?.Invoke(signal);
        }

        public override UnityHapticSignal Transmit(UnityHapticSignal signal, Float deltaTime)
        {

            WhileTransmitting?.Invoke(signal);
            return signal;

        }
    }
}