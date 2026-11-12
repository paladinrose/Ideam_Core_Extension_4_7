using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using System;

namespace Ideam.Unity.Science.Formal.Mathematics
{
    public class Spline3Behaviour : SplineBehaviour
    {
        public override event Action<int> OnSplinePointUpdated;

        public Spline3 spline;

        public virtual void SetPoint(int pointID, Vector3 pointValue)
        {
            if(pointID < 0 || pointID >= spline.Length) { return; }

            spline[pointID] = pointValue.ToFloat3();
            OnSplinePointUpdated?.Invoke(pointID);
        }
    }
}
