using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Ideam.Science.Applied.Electronics
{
    public interface IElectricalCircuit
    {
        float CurrentInput { get; }

        float CurrentThroughPut { get; }
        float CurrentUsage { get; }

        bool InputGate { get; set; }
        bool OutputGate { get; set; }

    }
}