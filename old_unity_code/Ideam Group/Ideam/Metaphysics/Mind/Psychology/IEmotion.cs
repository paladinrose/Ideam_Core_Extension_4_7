using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Metaphysics.Mind
{
    /// <summary>
    /// Represents a mental state characterized by a change in physiological and psychological states as the "feeling" of the emotion intensifies and/or diminishes.
    /// </summary>
    public interface IEmotion : IMentalState
    {
        T Feeling<T>() where T : IFeeling;
    }

    /// <summary>
    /// Represents the physiological and psychological changes that come from the "feeling" of an emotion.
    /// </summary>
    public interface IFeeling
    {
        
        event Action OnFeel;
    }
}