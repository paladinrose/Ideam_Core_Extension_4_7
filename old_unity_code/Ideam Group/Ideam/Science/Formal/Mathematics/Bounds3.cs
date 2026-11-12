using System;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Bounds3 :IBoundary<Bounds3, Float3>
    {

        public Bounds3(Float3 center, Float3 size)
        {
            this.size = size;
            extents = size * 0.5f;
            this.center = center;
            min = center - extents;
            max = center + extents;
        }

        Float3 center, size, min, max, extents;

        public Float3 Center
        {
            get { return center; }
            set
            {
                center = value;
                min = center - extents;
                max = center + extents;
            }
        }
        public Float3 Extents
        {
            get { return extents; }
        }

        public Float3 Size
        {
            get { return size; }
            set
            {
                size = value;
                extents = size * 0.5f;
            }
        }

        public Float3 Min
        {
            get { return min; }
            set { min = value; center = min + extents; }
        }

        public Float3 Max
        {
            get { return max; }
            set { max = value; center = max - extents; }
        }

        public Float3 ClosestPoint(Float3 point)
        {
            Float3 closest = center;

            return closest;
        }

        public bool Contains(Float3 point)
        {
            return ((point.x > min.x && point.y > min.y
                && point.z > min.z) &&
                (point.x < max.x && point.y < max.y
                && point.z < max.z));
        }

        public bool Contains(Bounds3 bounds)
        {
            if( Contains(bounds.Center) ||
                Contains(bounds.Min) ||
                Contains(bounds.Max)) { return true; }

            return false;
        }

        public void Encapsulate(Float3 point)
        {
            if (min.x > point.x) { min.x = point.x; }
            else if (max.x < point.x) { max.x = point.x; }

            if (min.y > point.y) { min.y = point.y; }
            else if (max.y < point.y) { max.y = point.y; }

            if (min.z > point.z) { min.z = point.z; }
            else if (max.z < point.z) { max.z = point.z; }

            center = (max + min) * 0.5f;
            size = max - min;
        }

        public void Encapsulate(Bounds3 other)
        {
            Encapsulate(other.min);
            Encapsulate(other.max);
        }

        public void Expand(Float3 expansion)
        {
            size = size + expansion;
        }

        public bool Intersects(Bounds3 other)
        {
            return false;
        }

        public void SetMinMax(Float3 min, Float3 max)
        {
            this.min = min; this.max = max;
            center = (max + min) * 0.5f;
            size = max - min;
        }


        public static bool operator ==(Bounds3 a, Bounds3 b)
        {
            if (a.center == b.center && a.size == b.size) { return true; }
            return false;
        }

        public static bool operator !=(Bounds3 a, Bounds3 b)
        {
            if (a.center != b.center || a.size == b.size) { return true; }
            return false;
        }

        public override bool Equals(object obj)
        {
            if (obj is Bounds3 b3)
            {
                return this == b3;
            }
            return false;
        }

        public bool Equals(Bounds3 other)
        {
            return this == other;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public override string ToString()
        {
            return "Center: " + center.ToString() + "  Size: " + size.ToString();
        }
    }
}
