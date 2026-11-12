using System;

using Ideam.Axiology.Valuation;

namespace Ideam.Science.Formal.Mathematics
{
    public interface IBoundary<TBound, T> :IEquatable<TBound>
        where TBound :IBoundary<TBound, T>
        where T : IValue<T>
    {
        T Center { get; }
        T Size { get; }
        T Min { get; }
        T Max { get; }
        //T ClosestPoint(T point);
        bool Contains(T point);
        bool Intersects(TBound bounds);

        void Encapsulate(T point);
        void Encapsulate(TBound bounds);
        void Expand(T expansion);
        void SetMinMax(T newMin, T newMax);
    }
}
