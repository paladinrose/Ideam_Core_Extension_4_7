using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Information;
using Ideam.Axiology.Valuation;

using Ideam.Unity.Metaphysics.Mind;

namespace Ideam.Unity.Science.Social.Communications
{
    public abstract class UnityCommunicator<TSignal> : UnityTransmitter<TSignal>
        where TSignal : ISignal
    {
        public UnityMind mind;

        
    }
}