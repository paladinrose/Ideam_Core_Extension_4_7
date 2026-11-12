using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Science.Formal.Mathematics.Information;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Metaphysics.Mind.Perception;

namespace Ideam.Metaphysics.Mind
{
    public interface IMind
    {
        int GetSenseID<T>(T sense) where T : ISense;
        T GetSense<T>(int id) where T : ISense;

        int GetMentalStateID<T>(T mentalState) where T : IMentalState;
        T GetMentalState<T>(int id) where T : IMentalState;

        int GetEmotionID<T>(T emotion) where T : IEmotion;
        T GetEmotion<T>(int id) where T : IEmotion;

    }
}
