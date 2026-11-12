using System;

namespace Ideam.Science.Formal.Mathematics
{

    [Serializable]
    public struct Bounds2 :IBoundary<Bounds2, Float2>
    {
        public Bounds2(Bounds2 other)
        {
            size = other.size;
            extents = other.extents;
            center = other.center;
            min = other.min;
            max = other.max;
        }

        public Bounds2(Float2 center, Float2 size)
        {
            this.size = size;
            extents = size * 0.5f;
            this.center = center;
            min = center - extents;
            max = center + extents;
        }

        Float2 center, size, min, max, extents;

        public Float2 Center
        {
            get { return center; }
            set
            {
                center = value;
                min = center - extents;
                max = center + extents;
            }
        }
        public Float2 Extents
        {
            get { return extents; }
        }

        public Float2 Size
        {
            get { return size; }
            set
            {
                size = value;
                extents = size * 0.5f;
            }
        }

        public Float2 Min
        {
            get { return min; }
            set { min = value; center = min + extents; }
        }

        public Float2 Max
        {
            get { return max; }
            set { max = value; center = max - extents; }
        }

        public Float2 ClosestPoint(Float2 point)
        {
            Float2 closest = center;

            return closest;
        }

        public bool Contains(Float2 point)
        {
            return ((point.x >= min.x && point.y >= min.y) &&
                (point.x <= max.x && point.y <= max.y));
        }

        public bool Contains(Bounds2 bounds)
        {
            if(Contains(bounds.Center) || Contains(bounds.Min) || Contains(bounds.Max)) { return true; }
            return false;
        }

        public void Encapsulate(Float2 point)
        {
            if (min.x > point.x) { min.x = point.x; }
            else if (max.x < point.x) { max.x = point.x; }

            if (min.y > point.y) { min.y = point.y; }
            else if (max.y < point.y) { max.y = point.y; }

            center = (max + min) * 0.5f;
            size = max - min;
            extents = size * 0.5f;
        }
        
        public void Encapsulate(Bounds2 other)
        {
            Encapsulate(other.min);
            Encapsulate(other.max);
        }

        public void Expand(Float2 expansion)
        {
            size = size + expansion;
        }

        public bool Intersects(Bounds2 other)
        {
            return false;
        }

        public void SetMinMax(Float2 min, Float2 max)
        {
            this.min = min; this.max = max;
            center = (max + min) * 0.5f;
            size = max - min;
            extents = size * 0.5f;
        }

        public static bool operator ==(Bounds2 a, Bounds2 b)
        {
            if(a.center == b.center && a.size == b.size) { return true; }
            return false;
        }

        public static bool operator !=(Bounds2 a, Bounds2 b)
        {
            if(a.center != b.center || a.size == b.size) { return true; }
            return false;
        }

        public override bool Equals(object obj)
        {
            if(obj is Bounds2 b2)
            {
                return this == b2;
            }
            return false;
        }

        public bool Equals(Bounds2 other) 
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
