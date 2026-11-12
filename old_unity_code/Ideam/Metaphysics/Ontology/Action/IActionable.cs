using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Axiology.Valuation;

namespace Ideam.Metaphysics.Ontology.Action
{

    public interface IActionable
    {
        Action<T>[] GetInitiatedActions<T>() where T : IValue<T>;
        Action<T>[] GetContinuousAndRepeatedActions<T>() where T : IValue<T>;
    }
}