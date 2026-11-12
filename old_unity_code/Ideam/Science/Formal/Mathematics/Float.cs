using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

namespace Ideam.Science.Formal.Mathematics
{

    [Serializable]
    public struct Float : IValue<Float>, IEquatable<float>, IDimensional<float>
    {
        public const float PI = (float)Math.PI;
        
        public const float Infinity = Single.PositiveInfinity;

        public const float NegativeInfinity = Single.NegativeInfinity;

        public const float DegToRad = PI * 2F / 360F;

        public const float RadToDeg = 1F / DegToRad;

        public static readonly float Epsilon = float.MinValue;

        public float x;

        public uint Dimensionality { get { return 1; } }

        #region Constructors

        public Float(float x)
        {
            this.x = x;
        }

        public Float(Float fromFloat)
        {
            x = fromFloat.x;
        }

        public static Float zero { get { return new Float(0f); } }
        public static Float one { get { return new Float(1f); } }

        

        #endregion

        #region Operators
        public Float Add(Float other) { return this + other; }
        public static Float operator +(Float a) { return new Float(Math.Abs(a.x)); }
        public static Float operator +(Float a, float b) { return new Float(a.x + b); }
        public static Float operator +(Float a, Float b) { return new Float(a.x + b.x); }
        public static Float operator ++(Float a) { return new Float(a.x + 1); }

        public Float Subtract(Float other) { return this - other; }
        public static Float operator -(Float a) { return new Float(-a.x); }
        public static Float operator -(Float a, float b) { return new Float(a.x - b); }
        public static Float operator -(Float a, Float b) { return new Float(a.x - b.x); }
        public static Float operator --(Float a) { return new Float(a.x - 1); }

        public Float Multiply(Float other) { return this * other; }
        public static Float operator *(Float a, float b) { return new Float(a.x * b); }
        public static Float operator *(Float a, Float b) { return new Float(a.x * b.x); }

        public Float Divide(Float other) { return this / other; }
        public static Float operator /(Float a, float b) { return new Float(a.x / b); }
        public static Float operator /(Float a, int b) { return new Float(a.x / b); }
        public static Float operator /(Float a, Float b) { return new Float(a.x / b.x); }

        public static Float operator %(Float a, float b) { return new Float(a.x % b); }
        public static Float operator %(Float a, Float b) { return new Float(a.x % b.x); }

        public static bool operator ==(Float a, Float b)
        {
            return a.x == b.x;
        }

        public static bool operator !=(Float a, Float b)
        {
            return !(a.x == b.x);
        }

        public static bool operator >(Float a, Float b)
        {
            return a.x > b.x;
        }

        public static bool operator >=(Float a, Float b)
        {
            return a.x >= b.x;
        }

        public static bool operator <(Float a, Float b)
        {
            return a.x < b.x;
        }
        public static bool operator <=(Float a, Float b)
        {
            return a.x <= b.x;
        }

        public override bool Equals(object obj)
        {
            return obj is Float newFloat && Equals(newFloat);
        }

        public bool Equals(Float other)
        {
            return x == other.x;
        }
        public bool Equals(float other)
        {
            return x == other;
        }

        public override int GetHashCode()
        {
            return x.GetHashCode();
        }
        
        public static implicit operator float(Float f) { return f.x; }
        public static implicit operator Float(float f) { return new Float(f); }

        public override string ToString()
        {
            return (x.ToString());
        }

        public bool TryConvert(object obj, out Float value)
        {
            if (obj is Float f) { value = f; return true; }
            else if (obj is float fl) { value = new Float(fl); return true; }
            value = 0;
            return false;
        }

        public bool NearlyEqual(Float point, float tolerance = float.MinValue)
        {
            Float diff = this - point;
            diff = +diff;
            if (diff.x > tolerance) { return false; }
            return true;
        }
        #endregion

        #region Methods

        public bool Compare(Float other, ValueComparison comparison = ValueComparison.EqualTo)
        {
            switch (comparison)
            {
                case ValueComparison.EqualTo: return this == other;

                case ValueComparison.NotEqual: return this != other;

                case ValueComparison.LessThan: return this.x < other.x;

                case ValueComparison.LessThanOrEqualTo: return this.x <= other.x;

                case ValueComparison.GreaterThan: return this.x > other.x;
                 
                case ValueComparison.GreaterThanOrEqualTo: return this.x >= other.x;
            }
            return false;
        }

        public static Float Lerp(Float start, Float end, float t)
        {
            return new Float
            (
                start.x + (end.x - start.x) * Clamp01(t)
            );
        }
        
        public static float Lerp(float start, float end, float t)
        {
            return start +(end - start) * Clamp01(t);
        }
        
        public static float InverseLerp(float a, float b, float value)
        {
            if (a != b)
                return Clamp01((value - a) / (b - a));
            else
                return 0.0f;
        }

        public static float LerpAngle(float a, float b, float t)
        {
            float delta = Repeat((b - a), 360);
            if (delta > 180)
                delta -= 360;
            return a + delta * Clamp01(t);
        }


        public static Float Average(Float[] values)
        {
            if (values.Length == 0) { return new Float(0f); }
            Float a = new Float(0f);
            for (int i = 0; i < values.Length; i++)
            {
                a += values[i];
            }
            return a / values.Length;
        }

        public static float Average(float[] values)
        {
            if(values.Length == 0) { return 0f; }
            float a = 0;
            for(int i = 0; i < values.Length; i++) 
            {
                a += values[i];
            }
            return a / values.Length;
        }


        public static float Sin(float f) { return (float)Math.Sin(f); }

        public static float Asin(float f) { return (float)Math.Asin(f); }

        
        public static float Cos(float f) { return (float)Math.Cos(f); }

        public static float Acos(float f) { return (float)Math.Acos(f); }

       
        public static float Tan(float f) { return (float)Math.Tan(f); }

        public static float Atan(float f) { return (float)Math.Atan(f); }

        public static float Atan2(float y, float x) { return (float)Math.Atan2(y, x); }

        
        public static float Sqrt(float f) { return (float)Math.Sqrt(f); }

        public Float Abs() { return new Float(Abs(x)); }

        public static float Abs(float f) { return (float)Math.Abs(f); }
        
        public static int Abs(int value) { return Math.Abs(value); }

        
        public static float Min(float a, float b) { return a < b ? a : b; }
        
        public static float Min(params float[] values)
        {
            int len = values.Length;
            if (len == 0)
                return 0;
            float m = values[0];
            for (int i = 1; i < len; i++)
            {
                if (values[i] < m)
                    m = values[i];
            }
            return m;
        }


        public static float Max(float a, float b) { return a > b ? a : b; }
        
        public static float Max(params float[] values)
        {
            int len = values.Length;
            if (len == 0)
                return 0;
            float m = values[0];
            for (int i = 1; i < len; i++)
            {
                if (values[i] > m)
                    m = values[i];
            }
            return m;
        }


        public static float Pow(float f, float p) { return (float)Math.Pow(f, p); }

        public static float Exp(float power) { return (float)Math.Exp(power); }

        
        public static float Log(float f, float p) { return (float)Math.Log(f, p); }

        public static float Log(float f) { return (float)Math.Log(f); }

        public static float Log10(float f) { return (float)Math.Log10(f); }


        public static float Ceiling(float f) { return (float)Math.Ceiling(f); }

        public static float Floor(float f) { return (float)Math.Floor(f); }

        public static float Round(float f) { return (float)Math.Round(f); }


        public static float Sign(float f) { return f >= 0F ? 1F : -1F; }


        public static float Clamp(float value, float min, float max)
        {
            if (value < min)
                value = min;
            else if (value > max)
                value = max;
            return value;
        }

        public static float Clamp01(float value)
        {
            if (value < 0F)
                return 0F;
            else if (value > 1F)
                return 1F;
            else
                return value;
        }


        public static float MoveTowards(float current, float target, float maxDelta)
        {
            if (Abs(target - current) <= maxDelta)
                return target;
            return current + Sign(target - current) * maxDelta;
        }

        public static float MoveTowardsAngle(float current, float target, float maxDelta)
        {
            float deltaAngle = DeltaAngle(current, target);
            if (-maxDelta < deltaAngle && deltaAngle < maxDelta)
                return target;
            target = current + deltaAngle;
            return MoveTowards(current, target, maxDelta);
        }


        public static float SmoothStep(float from, float to, float t)
        {
            t = Clamp01(t);
            t = -2.0F * t * t * t + 3.0F * t * t;
            return to * t + from * (1F - t);
        }
        
        public static float SmoothDamp(float current, float target, ref float currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
        {
            // Based on Game Programming Gems 4 Chapter 1.10
            smoothTime = Max(0.0001F, smoothTime);
            float omega = 2F / smoothTime;

            float x = omega * deltaTime;
            float exp = 1F / (1F + x + 0.48F * x * x + 0.235F * x * x * x);
            float change = current - target;
            float originalTo = target;

            // Clamp maximum speed
            float maxChange = maxSpeed * smoothTime;
            change = Clamp(change, -maxChange, maxChange);
            target = current - change;

            float temp = (currentVelocity + omega * change) * deltaTime;
            currentVelocity = (currentVelocity - omega * temp) * exp;
            float output = target + (change + temp) * exp;

            // Prevent overshooting
            if (originalTo - current > 0.0F == output > originalTo)
            {
                output = originalTo;
                currentVelocity = (output - originalTo) / deltaTime;
            }

            return output;
        }

        public static float SmoothDampAngle(float current, float target, ref float currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
        {
            target = current + DeltaAngle(current, target);
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime);
        }


        public static float Gamma(float value, float absmax, float gamma)
        {
            bool negative = value < 0F;
            float absval = Abs(value);
            if (absval > absmax)
                return negative ? -absval : absval;

            float result = Pow(absval / absmax, gamma) * absmax;
            return negative ? -result : result;
        }
        

        public static bool Approximately(float a, float b, float t)
        {
            return Abs(b - a) <= t;
        }

        public static bool Approximately(float a, float b)
        {
            return Abs(b - a) < Max(0.000001f * Max(Abs(a), Abs(b)), Epsilon * 8);
        }

        
        public static float Repeat(float t, float length)
        {
            return Clamp(t - Floor(t / length) * length, 0.0f, length);
        }

        public static float PingPong(float t, float length)
        {
            t = Repeat(t, length * 2F);
            return length - Abs(t - length);
        }

        
        public static float DeltaAngle(float current, float target)
        {
            float delta = Repeat((target - current), 360.0F);
            if (delta > 180.0F)
                delta -= 360.0F;
            return delta;
        }

        #endregion

        #region IDimensional Implementation
        
        public void SetDimension(int dimension, float value)
        {
            if(dimension != 0) { return; }
            x = value;
        }

        public float GetDimension(int dimension)
        {
            if(dimension != 0) { return default; }
            return x;
        }

        #endregion
    }
}
