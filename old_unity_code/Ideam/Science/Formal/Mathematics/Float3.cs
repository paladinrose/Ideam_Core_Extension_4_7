using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Float3 : IValue<Float3>, IDimensional<float>
    {
        public float x, y, z;
        public uint Dimensionality { get { return 3; } }

        #region Constructors

        public Float3(float x)
        {
            this.x = x;
            y = 0;
            z = 0;
        }

        public Float3(Float otherFloat)
        {
            x = otherFloat.x;
            y = 0;
            z = 0;
        }

        public Float3(float x, float y)
        {
            this.x = x;
            this.y = y;
            z = 0;
        }
        public Float3(Float2 otherFloat)
        {
            x = otherFloat.x;
            y = otherFloat.y;
            z = 0;
        }

        public Float3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }
        public Float3(Float3 otherFloat)
        {
            x = otherFloat.x;
            y = otherFloat.y;
            z = otherFloat.z;
        }

        public static Float3 zero { get { return new Float3(0, 0, 0); } }
        public static Float3 one { get { return new Float3(1, 1, 1); } }
        public static Float3 up { get { return new Float3(0, 1); } }
        public static Float3 down { get { return new Float3(0, -1); } }
        public static Float3 left { get { return new Float3(-1f); } }
        public static Float3 right { get { return new Float3(1f); } }
        public static Float3 front { get { return new Float3(0, 0, 1); } }
        public static Float3 back { get { return new Float3(0, 0, -1); } }
        #endregion

        #region Operators

        public Float3 Add(Float3 other) { return this + other; }
        public static Float3 operator +(Float3 a) { return new Float3(Math.Abs(a.x), Math.Abs(a.y), Math.Abs(a.z)); }
        public static Float3 operator +(Float3 a, float b) { return new Float3(a.x + b, a.y + b, a.z + b); }
        public static Float3 operator +(Float3 a, Float2 b) { return new Float3(a.x + b.x, a.y + b.y, a.z); }
        public static Float3 operator +(Float3 a, Float3 b) { return new Float3(a.x + b.x, a.y + b.y, a.z + b.z); }
        public static Float3 operator ++(Float3 a) { return new Float3(a.x + 1, a.y + 1, a.z + 1); }

        public Float3 Subtract(Float3 other) { return this - other; }
        public static Float3 operator -(Float3 a) { return new Float3(-a.x, -a.y, -a.z); }
        public static Float3 operator -(Float3 a, float b) { return new Float3(a.x - b, a.y - b, a.z - b); }
        public static Float3 operator -(Float3 a, Float2 b) { return new Float3(a.x - b.x, a.y - b.y, a.z); }
        public static Float3 operator -(Float3 a, Float3 b) { return new Float3(a.x - b.x, a.y - b.y, a.z - b.z); }
        public static Float3 operator --(Float3 a) { return new Float3(a.x - 1, a.y - 1, a.z - 1); }

        public Float3 Multiply(Float3 other) { return this * other; }
        public static Float3 operator *(Float3 a, float b) { return new Float3(a.x * b, a.y * b, a.z * b); }
        public static Float3 operator *(Float3 a, Float2 b) { return new Float3(a.x * b.x, a.y * b.y, a.z); }
        public static Float3 operator *(Float3 a, Float3 b) { return new Float3(a.x * b.x, a.y * b.y, a.z * b.z); }

        public Float3 Divide(Float3 other) { return this / other; }
        public static Float3 operator /(Float3 a, float b) { return new Float3(a.x / b, a.y / b, a.z / b); }
        public static Float3 operator /(Float3 a, Float2 b) { return new Float3(a.x / b.x, a.y / b.y, a.z); }
        public static Float3 operator /(Float3 a, Float3 b) { return new Float3(a.x / b.x, a.y / b.y, a.z / b.z); }

        public static Float3 operator %(Float3 a, float b) { return new Float3(a.x % b, a.y % b, a.z % b); }
        public static Float3 operator %(Float3 a, Float2 b) { return new Float3(a.x % b.x, a.y % b.y, a.z); }
        public static Float3 operator %(Float3 a, Float3 b) { return new Float3(a.x % b.x, a.y % b.y, a.z % b.z); }

        public static bool operator ==(Float3 a, Float3 b)
        {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }

        public static bool operator !=(Float3 a, Float3 b)
        {
            return !(a.x == b.x && a.y == b.y && a.z == b.z);
        }

        public override bool Equals(object obj)
        {
            return obj is Float3 newFloat && Equals(newFloat);
        }

        public bool Equals(Float3 other)
        {
            return x == other.x &&
                   y == other.y &&
                   z == other.z;
        }

        public override int GetHashCode()
        {
            int hashCode = 373119288;
            hashCode = hashCode * -1521134295 + x.GetHashCode();
            hashCode = hashCode * -1521134295 + y.GetHashCode();
            hashCode = hashCode * -1521134295 + z.GetHashCode();
            return hashCode;
        }

        public override string ToString()
        {
            return (x.ToString() + "," + y.ToString() + "," + z.ToString());
        }

        public bool TryConvert(object obj, out Float3 value)
        {
            if (obj is Float3 f) { value = f; return true; }
            else if (obj is UnityEngine.Vector3 v3) { value = new Float3(v3.x, v3.y, v3.z); return true; }
            value = Float3.zero;
            return false;
        }
        #endregion

        #region Methods

        public bool Compare(Float3 other, ValueComparison comparison = ValueComparison.EqualTo)
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

        public Float3 Abs() { return new Float3(Float.Abs(x), Float.Abs(y), Float.Abs(z)); }


        public static Float3 Lerp(Float3 start, Float3 end, float t)
        {
            float inv = 1f - t;
            return new Float3
            (
                start.x * inv - end.x * t,
                start.y * inv - end.y * t,
                start.z * inv - end.z * t
            );
        }

        public static Float3 Average(Float3[] values)
        {
            if (values.Length == 0) { return zero; }
            Float3 a = zero;
            for (int i = 0; i < values.Length; i++)
            {
                a += values[i];
            }
            return a / values.Length;
        }

        public static float Distance(Float3 a, Float3 b)
        {
            Float3 direction = a - b;
            return direction.Magnitude();
        }

        public float Magnitude() { return (float)Math.Sqrt(SquareMagnitude()); }

        public float SquareMagnitude()
        {
            return (x * x) +
                   (y * y) +
                   (z * z);
        }

        public Float3 Normalize()
        {
            float normalScalar = 1f;

            float biggestVal = Math.Abs(x);

            float abs = Math.Abs(y);
            if (abs > biggestVal) { biggestVal = abs; }

            abs = Math.Abs(z);
            if (abs > biggestVal) { biggestVal = abs; }

            if (biggestVal == 0) { return Float3.zero; }

            normalScalar /= biggestVal;
            return this * normalScalar;
        }

        public Float3 RotateAround(Float3 pivot, Float3 angles)
        {
            Float3 dir = this - pivot; // get point direction relative to pivot
            dir = Quaternion.Euler(angles) * dir; // rotate it
            Float3 point = dir + pivot; // calculate rotated point
            return point; // return it
        }

        public bool NearlyEqual(Float3 point, float tolerance = float.MinValue)
        {
            Float3 diff = this - point;
            diff = +diff;
            if (diff.x > tolerance) { return false; }
            if (diff.y > tolerance) { return false; }
            if (diff.z > tolerance) { return false; }
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
                default: return 0f;
            }
        }

        #endregion
    }
}
