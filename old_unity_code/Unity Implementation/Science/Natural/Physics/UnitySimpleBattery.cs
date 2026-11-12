using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Natural.Physics;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Natural.Physics
{
    [System.Serializable]
    public class UnitySimpleBattery : IBattery<Float>
    {
        [SerializeField]
        protected Float capacity;

        public Float Capacity { get { return capacity; } }

        [SerializeField]
        protected Float charge;

        public Float RemainingCharge { get { return charge; } }

        [SerializeField]
        protected Spline1 dischargeCurve;
        public TSpline DischargeCurve<TSpline>() where TSpline : SplineBase
        {
            if(dischargeCurve is TSpline dC)
            {
                return dC;
            }
            return default;
        }

        public Spline1 DischargeCurve()
        {
            return dischargeCurve;
        }

        public Float Discharge(Float pull)
        {
            Float p = charge/capacity;
            Float discharge = dischargeCurve.GetPoint(p)*pull;
            if (charge > discharge)
            {
                charge -= discharge;
            }
            else
            {
                discharge = charge;
                charge = Float.zero;
            }
            return discharge;

        }

        public void Charge(Float push)
        {
            throw new System.NotImplementedException();
        }
    }
}