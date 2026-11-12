using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Axiology.Valuation;
using Ideam.Epistemology.Semantics;
using Ideam.Metaphysics.Ontology.Existence;

using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Science.Applied.Cartography.Features
{
    public abstract class AbstractSpline2Feature : IFeature
    {
        public Spline2 footprint;

        #region IFeature Implentation

        public virtual T[] CoordinatesFootprint<T>() where T : IValue<T>
        {
            if (typeof(T) == typeof(Float2))
            {
                Float2[] p = footprint.SmoothedPoints;
                return p as T[];
            }

            return new T[0];
        }

        public abstract Type Represents();

        public abstract T Subject<T>() where T : IThing;

        #endregion

        #region ISemantic Implementation
        public virtual int AddSememe(int globalSememeID)
        {
            throw new NotImplementedException();
        }

        public int GetLocalSememeID(int globalSememeID)
        {
            throw new NotImplementedException();
        }
        public int[] GetSememeIDs()
        {
            return new int[0];
        }
        public int GetSememeID(int localID)
        {
            throw new NotImplementedException();
        }

        public T[] GetSememes<T>() where T : ISememe
        {
            throw new NotImplementedException();
        }

        public void RemoveSememe(int toRemove)
        {
            throw new NotImplementedException();
        }

        public T GetSemanticManager<T>() where T : ISemanticManager
        {
            throw new NotImplementedException();
        }

        public void SetSememeAtID(int newSememeID, int id)
        {
            throw new NotImplementedException();
        }

        public void CompareSemantics(ISemantic other, out int[] sharedSememes, out int[] missingSememes)
        {
            throw new NotImplementedException();
        }

        #endregion

    }
}