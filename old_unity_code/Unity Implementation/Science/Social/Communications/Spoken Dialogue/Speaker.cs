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

using Ideam.Unity.Metaphysics.Mind.Perception;

namespace Ideam.Unity.Science.Social.Communications
{
    public class Speaker : UnityCommunicator<UnityAudioSignal>
    {

        public override event Action<UnityAudioSignal> OnBeginTransmitting;
        public override event Action<UnityAudioSignal> WhileTransmitting;
        public override event Action<UnityAudioSignal> OnStopTransmitting;

        public override void BeginTransmission(UnityAudioSignal signal)
        {
            if (currentSignals.Contains(signal)) { return; }

            currentSignals.Add(signal);

            OnBeginTransmitting?.Invoke(signal);
        }

        public override void EndTransmission(UnityAudioSignal signal)
        {
            if (!currentSignals.Contains(signal)) { return; }

            currentSignals.Remove(signal);

            OnStopTransmitting?.Invoke(signal);
        }

        public override UnityAudioSignal Transmit(UnityAudioSignal signal, Float deltaTime)
        {

            WhileTransmitting?.Invoke(signal);
            return signal;

        }
    }
}