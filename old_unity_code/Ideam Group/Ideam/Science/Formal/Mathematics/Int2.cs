using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Int2 : IValue<Int2>, IDimensional<int>
    {
        public int x, y;
        public uint Dimensionality { get { return 2; } }

        #region Constructors

        public Int2(int x)
        {
            this.x = x;
            y = 0;
        }

        public Int2(Int fromInt)
        {
            x = fromInt.x;
            y = 0;
        }

        public Int2(int x, int y)
        {
            this.x = x;
            this.y = y;
        }

        public Int2(Int2 fromInt)
        {
            x = fromInt.x;
            y = fromInt.y;
        }

        public static Int2 zero { get { return new Int2(0, 0); } }
        public static Int2 one { get { return new Int2(1, 1); } }

        #endregion

        #region Operators

        public Int2 Add(Int2 other) { return this + other; }
        public static Int2 operator +(Int2 a) { return new Int2(Math.Abs(a.x), Math.Abs(a.y)); }
        public static Int2 operator +(Int2 a, int b) { return new Int2(a.x + b, a.y + b); }
        public static Int2 operator +(Int2 a, Int2 b) { return new Int2(a.x + b.x, a.y + b.y); }
        public static Int2 operator ++(Int2 a) { return new Int2(a.x + 1, a.y + 1); }

        public Int2 Subtract(Int2 other) { return this - other; }
        public static Int2 operator -(Int2 a) { return new Int2(-a.x, -a.y); }
        public static Int2 operator -(Int2 a, int b) { return new Int2(a.x - b, a.y - b); }
        public static Int2 operator -(Int2 a, Int2 b) { return new Int2(a.x - b.x, a.y - b.y); }
        public static Int2 operator --(Int2 a) { return new Int2(a.x - 1, a.y - 1); }

        public Int2 Multiply(Int2 other) { return this * other; }
        public static Int2 operator *(Int2 a, int b) { return new Int2(a.x * b, a.y * b); }
        public static Int2 operator *(Int2 a, Int2 b) { return new Int2(a.x * b.x, a.y * b.y); }

        public Int2 Divide(Int2 other) { return this / other; }
        public static Int2 operator /(Int2 a, int b) { return new Int2(a.x / b, a.y / b); }
        public static Int2 operator /(Int2 a, Int2 b) { return new Int2(a.x / b.x, a.y / b.y); }

        public static Int2 operator %(Int2 a, int b) { return new Int2(a.x % b, a.y % b); }
        public static Int2 operator %(Int2 a, Int2 b) { return new Int2(a.x % b.x, a.y % b.y); }

        public static bool operator ==(Int2 a, Int2 b)
        {
            return a.x == b.x && a.y == b.y;
        }
        
        public static bool operator !=(Int2 a, Int2 b)
        {
            return !(a.x == b.x && a.y == b.y);
        }

        public override bool Equals(object obj)
        {
            return obj is Int2 newInt && Equals(newInt);
        }

        public bool Equals(Int2 other)
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


        public bool TryConvert(object obj, out Int2 value)
        {
            if (obj is Int2 i) { value = i; return true; }
            else if (obj is UnityEngine.Vector2Int v2) { value = new Int2(v2.x, v2.y); return true; }
            value = Int2.zero;
            return false;
        }
        #endregion

        #region Methods

        public bool Compare(Int2 other, ValueComparison comparison = ValueComparison.EqualTo)
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

        public Int2 Abs() { return new Int2(Int.Abs(x), Int.Abs(y)); }
        #endregion

        #region IDimesional Implementation

        public void SetDimension(int dimension, int value)
        {
            switch (dimension)
            {
                case 0: x = value; break;
                case 1: y = value; break;
                default: return;
            }
        }

        public int GetDimension(int dimension)
        {
            switch (dimension)
            {
                case 0: return x;
                case 1: return y;
                default: return 0;
            }
        }

        #endregion
    }
}
