using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Applied.ProceduralGeneration 
{
    public class ProceduralSpline2Asset : ProceduralAsset
    {
        public Spline2[] splines;

        public Spline2 GetSplineCopy(int id) 
        {
            Spline2 copy = new Spline2(splines[id]);
            return copy;
        }


    }
}