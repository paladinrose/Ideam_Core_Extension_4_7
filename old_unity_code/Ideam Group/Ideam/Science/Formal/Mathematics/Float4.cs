using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Float4 : IValue<Float4>, IDimensional<float>
    {
        public float x, y, z, w;
        public uint Dimensionality { get { return 4; } }

        #region Constructors

        public Float4(float x)
        {
            this.x = x;
            y = 0;
            z = 0;
            w = 0;
        }
        public Float4(Float otherFloat)
        {
            x = otherFloat.x;
            y = 0;
            z = 0;
            w = 0;
        }

        public Float4(float x, float y)
        {
            this.x = x;
            this.y = y;
            z = 0;
            w = 0;
        }
        public Float4(Float2 otherFloat)
        {
            x = otherFloat.x;
            y = otherFloat.y;
            z = 0;
            w = 0;
        }

        public Float4(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            w = 0;
        }
        public Float4(Float3 otherFloat)
        {
            x = otherFloat.x;
            y = otherFloat.y;
            z = otherFloat.z;
            w = 0;
        }

        public Float4(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }
        public Float4(Float4 otherFloat)
        {
            x = otherFloat.x;
            y = otherFloat.y;
            z = otherFloat.z;
            w = otherFloat.w;
        }

        public static Float4 zero { get { return new Float4(0, 0, 0, 0); } }
        public static Float4 one { get { return new Float4(1, 1, 1, 1); } }
        public static Float4 up { get { return new Float4(0, 1); } }
        public static Float4 down { get { return new Float4(0, -1); } }
        public static Float4 left { get { return new Float4(-1f); } }
        public static Float4 right { get { return new Float4(1f); } }
        public static Float4 front { get { return new Float4(0, 0, 1); } }
        public static Float4 back { get { return new Float4(0, 0, -1); } }
        public static Float4 fore { get { return new Float4(0, 0, 0, 1); } }
        public static Float4 aft { get { return new Float4(0, 0, 0, -1); } }

        #endregion

        #region Operators

        public Float4 Add(Float4 other) { return this + other; }
        public static Float4 operator +(Float4 a) { return new Float4(Math.Abs(a.x), Math.Abs(a.y), Math.Abs(a.z), Math.Abs(a.w)); }
        public static Float4 operator +(Float4 a, float b) { return new Float4(a.x + b, a.y + b, a.z + b, a.w + b); }
        public static Float4 operator +(Float4 a, Float2 b) { return new Float4(a.x + b.x, a.y + b.y, a.z, a.w); }
        public static Float4 operator +(Float4 a, Float3 b) { return new Float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w); }
        public static Float4 operator +(Float4 a, Float4 b) { return new Float4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
        public static Float4 operator ++(Float4 a) { return new Float4(a.x + 1, a.y + 1, a.z + 1, a.w + 1); }

        public Float4 Subtract(Float4 other) { return this - other; }
        public static Float4 operator -(Float4 a) { return new Float4(-a.x, -a.y, -a.z, -a.w); }
        public static Float4 operator -(Float4 a, float b) { return new Float4(a.x - b, a.y - b, a.z - b, a.w - b); }
        public static Float4 operator -(Float4 a, Float2 b) { return new Float4(a.x - b.x, a.y - b.y, a.z, a.w); }
        public static Float4 operator -(Float4 a, Float3 b) { return new Float4(a.x - b.x, a.y - b.y, a.z - b.z, a.w); }
        public static Float4 operator -(Float4 a, Float4 b) { return new Float4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
        public static Float4 operator --(Float4 a) { return new Float4(a.x - 1, a.y - 1, a.z - 1, a.w - 1); }

        public Float4 Multiply(Float4 other) { return this * other; }
        public static Float4 operator *(Float4 a, float b) { return new Float4(a.x * b, a.y * b, a.z * b, a.w * b); }
        public static Float4 operator *(Float4 a, Float2 b) { return new Float4(a.x * b.x, a.y * b.y, a.z, a.w); }
        public static Float4 operator *(Float4 a, Float3 b) { return new Float4(a.x * b.x, a.y * b.y, a.z * b.z, a.w); }
        public static Float4 operator *(Float4 a, Float4 b) { return new Float4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }

        public Float4 Divide(Float4 other) { return this / other; }
        public static Float4 operator /(Float4 a, float b) { return new Float4(a.x / b, a.y / b, a.z / b, a.w / b); }
        public static Float4 operator /(Float4 a, Float2 b) { return new Float4(a.x / b.x, a.y / b.y, a.z, a.w); }
        public static Float4 operator /(Float4 a, Float3 b) { return new Float4(a.x / b.x, a.y / b.y, a.z / b.z, a.w); }
        public static Float4 operator /(Float4 a, Float4 b) { return new Float4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }

        public static Float4 operator %(Float4 a, float b) { return new Float4(a.x % b, a.y % b, a.z % b, a.w % b); }
        public static Float4 operator %(Float4 a, Float2 b) { return new Float4(a.x % b.x, a.y % b.y, a.z, a.w); }
        public static Float4 operator %(Float4 a, Float3 b) { return new Float4(a.x % b.x, a.y % b.y, a.z % b.z, a.w); }
        public static Float4 operator %(Float4 a, Float4 b) { return new Float4(a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w); }

        public static bool operator ==(Float4 a, Float4 b)
        {
            return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
        }

        public static bool operator !=(Float4 a, Float4 b)
        {
            return !(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
        }

        public override bool Equals(object obj)
        {
            return obj is Float4 newFloat && Equals(newFloat);
        }

        public bool Equals(Float4 other)
        {
            return x == other.x &&
                   y == other.y &&
                   z == other.z &&
                   w == other.w;
        }

        public override int GetHashCode()
        {
            int hashCode = -1743314642;
            hashCode = hashCode * -1521134295 + x.GetHashCode();
            hashCode = hashCode * -1521134295 + y.GetHashCode();
            hashCode = hashCode * -1521134295 + z.GetHashCode();
            hashCode = hashCode * -1521134295 + w.GetHashCode();
            return hashCode;
        }

        public override string ToString()
        {
            return (x.ToString() + "," + y.ToString() + "," + z.ToString() + "," + w.ToString());
        }

        public bool TryConvert(object obj, out Float4 value)
        {
            if (obj is Float4 f) { value = f; return true; }
            else if (obj is UnityEngine.Vector4 v4) { value = new Float4(v4.x, v4.y, v4.z,v4.w); return true; }
            value = Float4.zero;
            return false;
        }
        #endregion

        #region Methods

        public bool Compare(Float4 other, ValueComparison comparison = ValueComparison.EqualTo)
        {
            switch (comparison)
            {
                case ValueComparison.EqualTo: return this == other;

                case ValueComparison.NotEqual: return this != other;

                case ValueComparison.LessThan:
                    break;
                case ValueComparison.LessThanOrEqualTo:
                    break;
                case ValueComparison.GreaterThan:
                    break;
                case ValueComparison.GreaterThanOrEqualTo:
                    break;
            }
            return false;
        }

        public Float4 Abs() { return new Float4(Float.Abs(x), Float.Abs(y), Float.Abs(z), Float.Abs(w)); }

        public static Float4 RGBToHSV(Float4 rgb)
        {
            Float4 hsv = new Float4
            (
                rgb.x, 
                rgb.y, 
                rgb.z, 
                rgb.w
            );

            return hsv;
        }

        public static Float4 Lerp(Float4 start, Float4 end, float t)
        {
            float inv = 1f - t;
            return new Float4
            (
                start.x * inv - end.x * t,
                start.y * inv - end.y * t,
                start.z * inv - end.z * t,
                start.w *inv - end.w*t
            );
        }

        public static Float4 Average(Float4[] values)
        {
            if (values.Length == 0) { return zero; }
            Float4 a = zero;
            for (int i = 0; i < values.Length; i++)
            {
                a += values[i];
            }
            return a / values.Length;
        }

        public static float Distance(Float4 a, Float4 b)
        {
            Float4 direction = a - b;
            return direction.Magnitude();
        }

        public float Magnitude() { return (float)Math.Sqrt(SquareMagnitude()); }

        public float SquareMagnitude()
        {
            return ( x * x ) +
                   ( y * y ) +
                   ( z * z ) +
                   ( w * w );
        }

        public Float4 Normalize()
        {
            float normalScalar = 1f;

            float biggestVal = Math.Abs(x);
            float abs = Math.Abs(y);

            if (abs > biggestVal) { biggestVal = abs; }

            abs = Math.Abs(z);
            if(abs > biggestVal) { biggestVal = abs; }

            abs = Math.Abs(w);
            if(abs > biggestVal) { biggestVal = abs; }

            if(biggestVal == 0) { return Float4.zero; }

            normalScalar /= biggestVal;
            return this * normalScalar;
        }

        public bool NearlyEqual(Float4 point, float tolerance = float.MinValue)
        {
            Float4 diff = this - point;
            diff = +diff;
            if (diff.x > tolerance) { return false; }
            if (diff.y > tolerance) { return false; }
            if (diff.z > tolerance) { return false; }
            if (diff.w > tolerance) { return false; }

            return true;
        }
        #endregion

        #region IDimesional Implementation

        public void SetDimension(int dimension, float value)
        {
            switch (dimension)
            {
                case 0: x = value; break;
                case 1: y = value; break;
                case 2: z = value; break;
                case 3: w = value; break;
                default: return;
            }
        }

        public float GetDimension(int dimension)
        {
            switch (dimension)
            {
                case 0: return x;
                case 1: return y;
                case 2: return z;
                case 3: return w;
                default: return 0f;
            }
        }

        #endregion
    }
}
