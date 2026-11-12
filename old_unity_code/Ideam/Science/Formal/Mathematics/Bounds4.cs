using System;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Bounds4 :IBoundary<Bounds4, Float4>
    {

        public Bounds4(Float4 center, Float4 size)
        {
            this.size = size;
            extents = size * 0.5f;
            this.center = center;
            min = center - extents;
            max = center + extents;
        }

        Float4 center, size, min, max, extents;

        public Float4 Center
        {
            get { return center; }
            set
            {
                center = value;
                min = center - extents;
                max = center + extents;
            }
        }
        public Float4 Extents
        {
            get { return extents; }
        }

        public Float4 Size
        {
            get { return size; }
            set
            {
                size = value;
                extents = size * 0.5f;
            }
        }

        public Float4 Min
        {
            get { return min; }
            
        }

        public Float4 Max
        {
            get { return max; }
           
        }
        
        public Float4 ClosestPoint(Float4 point)
        {
            Float4 closest = center;

            return closest;
        }

        public bool Contains(Float4 point)
        {
            return ((point.x > min.x && point.y > min.y
                && point.z > min.z && point.w > min.w) &&
                (point.x < max.x && point.y < max.y
                && point.z < max.z && point.w < max.w));
        }

        public void Encapsulate(Float4 point)
        {
            if (min.x > point.x) { min.x = point.x; }
            else if (max.x < point.x) { max.x = point.x; }

            if (min.y > point.y) { min.y = point.y; }
            else if (max.y < point.y) { max.y = point.y; }

            if (min.z > point.z) { min.z = point.z; }
            else if (max.z < point.z) { max.z = point.z; }

            if (min.w > point.w) { min.w = point.w; }
            else if (max.w < point.w) { max.w = point.w; }

            center = (max + min) * 0.5f;
            size = max - min;
        }

        public void Encapsulate(Bounds4 other)
        {
            Encapsulate(other.min);
            Encapsulate(other.max);
        }

        public void Expand(Float4 expansion)
        {
            size = size + expansion;
        }

        public bool Intersects(Bounds4 other)
        {
            return false;
        }

        public void SetMinMax(Float4 min, Float4 max)
        {
            this.min = min; this.max = max;
            center = (max + min) * 0.5f;
            size = max - min;
        }

        public static bool operator ==(Bounds4 a, Bounds4 b)
        {
            if (a.center == b.center && a.size == b.size) { return true; }
            return false;
        }

        public static bool operator !=(Bounds4 a, Bounds4 b)
        {
            if (a.center != b.center || a.size == b.size) { return true; }
            return false;
        }

        public override bool Equals(object obj)
        {
            if (obj is Bounds4 b4)
            {
                return this == b4;
            }
            return false;
        }

        public bool Equals(Bounds4 other)
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
