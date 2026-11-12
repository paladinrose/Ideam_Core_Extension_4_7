using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Float2 : IValue<Float2>, IDimensional<float> 
    {
        public float x, y;
        public uint Dimensionality { get { return 2; } }

        #region Constructors

        public Float2(float x)
        {
            this.x = x;
            y = 0;
        }
        public Float2(Float otherFloat)
        {
            x = otherFloat.x;
            y = 0;
        }

        public Float2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }
        public Float2(Float2 otherFloat)
        {
            x = otherFloat.x;
            y = otherFloat.y;
        }

        public static Float2 zero { get { return new Float2(0, 0); } }
        public static Float2 one { get { return new Float2(1, 1); } }
        public static Float2 up { get { return new Float2(0, 1); } }
        public static Float2 down { get { return new Float2(0, -1); } }
        public static Float2 left { get { return new Float2(-1f); } }
        public static Float2 right { get { return new Float2(1f); } }

        
        #endregion

        #region Operators

        public Float2 Add(Float2 other) { return this + other; }
        public static Float2 operator +(Float2 a) { return new Float2(Math.Abs(a.x), Math.Abs(a.y)); }
        public static Float2 operator +(Float2 a, float b) { return new Float2(a.x + b, a.y + b); }
        public static Float2 operator +(Float2 a, Float2 b) { return new Float2(a.x + b.x, a.y + b.y); }
        public static Float2 operator ++(Float2 a) { return new Float2(a.x + 1, a.y + 1); }

        public Float2 Subtract(Float2 other) { return this - other; }
        public static Float2 operator -(Float2 a) { return new Float2(-a.x, -a.y); }
        public static Float2 operator -(Float2 a, float b) { return new Float2(a.x - b, a.y - b); }
        public static Float2 operator -(Float2 a, Float2 b) { return new Float2(a.x - b.x, a.y - b.y); }
        public static Float2 operator --(Float2 a) { return new Float2(a.x - 1, a.y - 1); }

        public Float2 Multiply(Float2 other) { return this * other; }
        public static Float2 operator *(Float2 a, float b) { return new Float2(a.x * b, a.y * b); }
        public static Float2 operator *(Float2 a, Float2 b) { return new Float2(a.x * b.x, a.y * b.y); }

        public Float2 Divide(Float2 other) { return this / other; }
        public static Float2 operator /(Float2 a, float b) { return new Float2(a.x / b, a.y / b); }
        public static Float2 operator /(Float2 a, Float2 b) { return new Float2(a.x / b.x, a.y / b.y); }

        public static Float2 operator %(Float2 a, float b) { return new Float2(a.x % b, a.y % b); }
        public static Float2 operator %(Float2 a, Float2 b) { return new Float2(a.x % b.x, a.y % b.y); }

        public static bool operator ==(Float2 a, Float2 b)
        {
            return a.x == b.x && a.y == b.y;
        }

        public static bool operator !=(Float2 a, Float2 b)
        {
            return !(a.x == b.x && a.y == b.y);
        }

        public override bool Equals(object obj)
        {
            return obj is Float2 newFloat && Equals(newFloat);
        }

        public bool Equals(Float2 other)
        {
            return x == other.x &&
                   y == other.y;
        }

        public override int GetHashCode()
        {
            int hashCode = 1502939027;
            hashCode = hashCode * -1521134295 + x.GetHashCode();
            hashCode = hashCode * -1521134295 + y.GetHashCode();
            return hashCode;
        }

        public override string ToString()
        {
            return (x.ToString() +"," + y.ToString());
        }

        public bool TryConvert(object obj, out Float2 value)
        {
            if(obj is Float2 f) { value = f; return true; }
            else if(obj is UnityEngine.Vector2 v2) { value = new Float2(v2.x, v2.y); return true; }
            value = Float2.zero;
            return false;
        }
        #endregion

        #region Methods

        public bool Compare(Float2 other, ValueComparison comparison = ValueComparison.EqualTo)
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

        public static Float2 Lerp(Float2 start, Float2 end, float t)
        {
            return new Float2
            (
                Float.Lerp(start.x, end.x, t),
                Float.Lerp(start.y, end.y, t)
            );
        }


        public static Float2 Average(Float2[] values)
        {
            if (values.Length == 0) { return zero; }
            Float2 a = zero;
            for (int i = 0; i < values.Length; i++)
            {
                a += values[i];
            }
            return a / values.Length;
        }

        public static float Sign(Float2 p1, Float2 p2, Float2 p3)
        {
            return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
        }

        public static bool InsideTriangle(Float2 A, Float2 B, Float2 C, Float2 P)
        {
           
            float denominator = ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
            float a = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denominator;
            float b = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denominator;
            float c = 1f - a - b;

            return 0f <= a && a <= 1f && 0f <= b && b <= 1f && 0f <= c && c <= 1f;
           
            /*
            float d1, d2, d3;
            bool has_neg, has_pos;

            d1 = Sign(P, A, B);
            d2 = Sign(P, B, C);
            d3 = Sign(P, C, A);

            has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            return !(has_neg && has_pos);
            */
        }

        public static float ManhattanDistance(Float2 a, Float2 b)
        {
            return ((Float.Abs(a.x - b.x)) + Float.Abs(a.y - b.y));
        }

        public static float Distance(Float2 a, Float2 b)
        {
            Float2 direction = a - b;
            return direction.Magnitude();
        }

        public static Float2 Perpendicular(Float2 direction)
        {
            return new Float2(-direction.y, direction.x);
        }

        public Float2 Abs() { return new Float2(Float.Abs(x), Float.Abs(y)); }

        public static float Dot(Float2 lhs, Float2 rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }

        public static float Cross(Float2 a, Float2 b)
        {
            return (a.x * b.y) - (a.y * b.x);
        }

        public static float Angle(Float2 from, Float2 to)
        {
            float denominator = (float)Math.Sqrt(from.SquareMagnitude() * to.SquareMagnitude());
            //if (denominator < kEpsilonNormalSqrt)
                //return 0F;

            float dot = Float.Clamp(Dot(from, to) / denominator, -1F, 1F);
            return Float.Acos(dot) * Float.RadToDeg;
        }

        public static float SignedAngle(Float2 from, Float2 to)
        {
            float unsigned_angle = Angle(from, to);
            float sign = Math.Sign(from.x * to.y - from.y * to.x);
            return unsigned_angle * sign;
        }

        public bool NearlyEqual(Float2 point, float tolerance = float.MinValue)
        {
            Float2 diff = this - point;
            diff = +diff;
            if(diff.x > tolerance) { return false; }
            if(diff.y > tolerance) { return false; }
            return true;
        }

        public float Magnitude() { return (float)Math.Sqrt(SquareMagnitude()); }

        public float SquareMagnitude()
        {
            return (x * x) +
                   (y * y);
        }

        public Float2 Normalize()
        {
            float normalScalar = 1f;

            float biggestVal = Math.Abs(x);
            float abs = Math.Abs(y);

            if (abs > biggestVal) { biggestVal = abs; }
            
            if (biggestVal == 0) { return Float2.zero; }

            normalScalar /= biggestVal;
            return this * normalScalar;
        }

        public bool IsOnLine(Float2 lineStart, Float2 lineEnd)
        {
            float slope = (lineEnd.y - lineStart.y) / (lineEnd.x - lineStart.x);
            float yIntercept = lineStart.y - lineStart.x * slope;
            return ((slope * x + yIntercept) == (y));
        }

        public static bool LineIntersection(Float2 pA1, Float2 pA2, Float2 pB1, Float2 pB2, ref Float2 intersection)
        {
            if (pA1.Equals(pB1) || pA1.Equals(pB2) || pA2.Equals(pB1) || pA2.Equals(pB2)) { return false; }

            float Ax, Bx, Cx, Ay, By, Cy, alphaNumerator, betaNumerator, denominator, intersectNumerator;
            float x1lo, x1hi, y1lo, y1hi;

            Ax = pA2.x - pA1.x;
            Bx = pB1.x - pB2.x;

            // X bound box test/

            if (Ax < 0) { x1lo = pA2.x; x1hi = pA1.x; }
            else { x1hi = pA2.x; x1lo = pA1.x; }

            if (Bx > 0) { if (x1hi < pB2.x || pB1.x < x1lo) { return false; } }
            else { if (x1hi < pB1.x || pB2.x < x1lo) { return false; } }

            Ay = pA2.y - pA1.y;
            By = pB1.y - pB2.y;

            // Y bound box test//

            if (Ay < 0) { y1lo = pA2.y; y1hi = pA1.y; }
            else { y1hi = pA2.y; y1lo = pA1.y; }

            if (By > 0) { if (y1hi < pB2.y || pB1.y < y1lo) { return false; } }
            else { if (y1hi < pB1.y || pB2.y < y1lo) { return false; } }

            Cx = pA1.x - pB1.x;
            Cy = pA1.y - pB1.y;

            // alpha tests

            alphaNumerator = By * Cx - Bx * Cy;
            denominator = Ay * Bx - Ax * By;

            if (denominator > 0) { if (alphaNumerator < 0 || alphaNumerator > denominator) { return false; } }
            else { if (alphaNumerator > 0 || alphaNumerator < denominator) { return false; } }

            // Beta tests 

            betaNumerator = Ax * Cy - Ay * Cx;

            if (denominator > 0) { if (betaNumerator < 0 || betaNumerator > denominator) { return false; } }
            else { if (betaNumerator > 0 || betaNumerator < denominator) { return false; } }

            // Check if they are parallel

            if (denominator == 0) { return false; }

            // Compute intersection coordinates 

            intersectNumerator = alphaNumerator * Ax;

            intersection.x = pA1.x + intersectNumerator / denominator;

            intersectNumerator = alphaNumerator * Ay;

            intersection.y = pA1.y + intersectNumerator / denominator;

            return true;
        }

        public Float2 RotateAround(Float2 pivot, float angle)
        {
            if (angle == 0) { return this; }
            Float2 p = this - pivot;
            float rad = angle * Float.DegToRad;
            float s = Float.Sin(rad);
            float c = Float.Cos(rad);
            return pivot + new Float2(p.x * c - p.y * s, p.y * c + p.x * s);

        }


        
        #endregion

        #region IDimesional Implementation
        
        public void SetDimension(int dimension, float value)
        {
            switch (dimension)
            {
                case 0: x = value; break;
                case 1: y = value; break;
                default:return;
            }
        }

        public float GetDimension(int dimension)
        {
            switch (dimension)
            {
                case 0: return x;
                case 1: return y;
                default:return 0f;
            }
        }

        public Float2 Perpendicular()
        {
            return new Float2(-y, x) / Magnitude();
        }



        #endregion
    }
}
