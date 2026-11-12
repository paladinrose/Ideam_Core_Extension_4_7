using System;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Bounds : IBoundary<Bounds, Float>
    {
        public Bounds(Bounds other)
        {
            size = other.size;
            extents = other.extents;
            center = other.center;
            min = other.min;
            max = other.max;
        }

        public Bounds(Float center, Float size)
        {
            this.size = size;
            extents = size * 0.5f;
            this.center = center;
            min = center - extents;
            max = center + extents;
        }

        Float center, size, min, max, extents;

        public Float Center
        {
            get { return center; }
            set
            {
                center = value;
                min = center - extents;
                max = center + extents;
            }
        }
        public Float Extents
        {
            get { return extents; }
        }

        public Float Size
        {
            get { return size; }
            set
            {
                size = value;
                extents = size * 0.5f;
            }
        }

        public Float Min
        {
            get { return min; }
            set { min = value; center = min + extents; }
        }

        public Float Max
        {
            get { return max; }
            set { max = value; center = max - extents; }
        }

        public Float ClosestPoint(Float point)
        {
            Float closest = center;

            return closest;
        }

        public bool Contains(Float point)
        {
            return ((point.x >= min.x  &&point.x <= max.x));
        }

        public void Encapsulate(Float point)
        {
            if (min.x > point.x) { min.x = point.x; }
            else if (max.x < point.x) { max.x = point.x; }

            center = (max + min) * 0.5f;
            size = max - min;
            extents = size * 0.5f;
        }

        public void Encapsulate(Bounds other)
        {
            Encapsulate(other.min);
            Encapsulate(other.max);
        }

        public void Expand(Float expansion)
        {
            size = size + expansion;
        }

        public bool Intersects(Bounds other)
        {
            return false;
        }

        public void SetMinMax(Float min, Float max)
        {
            this.min = min; this.max = max;
            center = (max + min) * 0.5f;
            size = max - min;
            extents = size * 0.5f;
        }

        public static bool operator ==(Bounds a, Bounds b)
        {
            if (a.center == b.center && a.size == b.size) { return true; }
            return false;
        }

        public static bool operator !=(Bounds a, Bounds b)
        {
            if (a.center != b.center || a.size == b.size) { return true; }
            return false;
        }

        public override bool Equals(object obj)
        {
            if (obj is Bounds b2)
            {
                return this == b2;
            }
            return false;
        }

        public bool Equals(Bounds other)
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
