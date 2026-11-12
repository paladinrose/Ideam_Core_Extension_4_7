using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Int : IValue<Int>, IEquatable<int>, IDimensional<int>
    {
        public int x;
        public uint Dimensionality { get { return 1; } }

        #region Constructors

        public Int(int x)
        {
            this.x = x;
        }

        public Int(Int fromInt)
        {
            x = fromInt.x;
        }

        public static Int zero { get { return new Int(0); } }
        public static Int one { get { return new Int(1); } }

        #endregion

        #region Operators

        public Int Add(Int other) { return this + other; }
        public static Int operator +(Int a) { return new Int(Math.Abs(a.x)); }
        public static Int operator +(Int a, int b) { return new Int(a.x + b); }
        public static Int operator +(Int a, Int b) { return new Int(a.x + b.x); }
        public static Int operator ++(Int a) { return new Int(a.x + 1); }

        public Int Subtract(Int other) { return this - other; }
        public static Int operator -(Int a) { return new Int(-a.x); }
        public static Int operator -(Int a, int b) { return new Int(a.x - b); }
        public static Int operator -(Int a, Int b) { return new Int(a.x - b.x); }
        public static Int operator --(Int a) { return new Int(a.x - 1); }

        public Int Multiply(Int other) { return this * other; }
        public static Int operator *(Int a, int b) { return new Int(a.x * b); }
        public static Int operator *(Int a, Int b) { return new Int(a.x * b.x); }

        public Int Divide(Int other) { return this / other; }
        public static Int operator /(Int a, int b) { return new Int(a.x / b); }
        public static Int operator /(Int a, Int b) { return new Int(a.x / b.x); }

        public static Int operator %(Int a, int b) { return new Int(a.x % b); }
        public static Int operator %(Int a, Int b) { return new Int(a.x % b.x); }

        public static bool operator ==(Int a, Int b)
        {
            return a.x == b.x;
        }

        public static bool operator !=(Int a, Int b)
        {
            return !(a.x == b.x);
        }

        public static bool operator >(Int a, Int b)
        {
            return a.x > b.x;
        }

        public static bool operator >=(Int a, Int b)
        {
            return a.x >= b.x;
        }

        public static bool operator <(Int a, Int b)
        {
            return a.x < b.x;
        }
        public static bool operator <=(Int a, Int b)
        {
            return a.x <= b.x;
        }

        public override bool Equals(object obj)
        {
            return obj is Int newInt && Equals(newInt);
        }

        public bool Equals(Int other)
        {
            return x == other.x;
        }

        public bool Equals(int other)
        {
            return x == other;
        }

        public static implicit operator int(Int i)
        {
            return i.x;
        }
        public static implicit operator Int(int i)
        {
            return new Int(i);
        }
        public override int GetHashCode()
        {
            return 1569241247 + x.GetHashCode();
        }


        public bool TryConvert(object obj, out Int value)
        {
            if (obj is Int i) { value = i; return true; }
            else if (obj is int ii) { value = ii; return true; }
            value = 0;
            return false;
        }
        #endregion

        #region Methods

        public bool Compare(Int other, ValueComparison comparison = ValueComparison.EqualTo)
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

        public Int Abs() { return new Int(Abs(x)); }
        public static int Abs(int a) { return Math.Abs(a); }

        public static int Min(int a, int b) { return a < b ? a : b; }
        
        public static int Min(params int[] values)
        {
            int len = values.Length;
            if (len == 0)
                return 0;
            int m = values[0];
            for (int i = 1; i < len; i++)
            {
                if (values[i] < m)
                    m = values[i];
            }
            return m;
        }


        public static int Max(int a, int b) { return a > b ? a : b; }
        
        public static int Max(params int[] values)
        {
            int len = values.Length;
            if (len == 0)
                return 0;
            int m = values[0];
            for (int i = 1; i < len; i++)
            {
                if (values[i] > m)
                    m = values[i];
            }
            return m;
        }
        

        public static int Ceiling(float f) { return (int)Math.Ceiling(f); }

        public static int Floor(float f) { return (int)Math.Floor(f); }

        public static int Round(float f) { return (int)Math.Round(f); }
       

        public static int Clamp(int value, int min, int max)
        {
            if (value < min)
                value = min;
            else if (value > max)
                value = max;
            return value;
        }
        #endregion

        #region IDimensional Implementation

        public void SetDimension(int dimension, int value)
        {
            if (dimension != 0) { return; }
            x = value;
        }

        public int GetDimension(int dimension)
        {
            if (dimension != 0) { return default; }
            return x;
        }

        #endregion
    }
}
