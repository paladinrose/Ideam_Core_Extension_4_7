using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Science.Formal.Mathematics.Information;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Metaphysics.Mind.Perception
{
    public interface ISense : IReceiver, IDecoder
    {
        void BeginSensing<T>(T sensedThing) where T : IThing;
        void Sensing<T>(T sensedThing) where T : IThing;
        void StopSensing<T>(T notSensedThing) where T : IThing;

        void NewInformation(List<Information> newInfoList);
    }
}
