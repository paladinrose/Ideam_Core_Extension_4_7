using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Formal.Mathematics
{
    public class SplineBehaviour : MonoBehaviour
    {
        public virtual event Action<int> OnSplinePointUpdated;
    }
}