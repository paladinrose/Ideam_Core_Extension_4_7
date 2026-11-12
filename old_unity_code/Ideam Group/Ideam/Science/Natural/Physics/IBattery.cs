using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Science.Natural.Physics
{
    public interface IBattery<T>
        where T:IValue<T>
    {
        public T Capacity { get; }

        public T RemainingCharge { get; }

        public TSpline DischargeCurve<TSpline>() where TSpline : SplineBase;

        public T Discharge(T pull);

        public void Charge(T push);
    }
}

