using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Int4 : IValue<Int4>, IDimensional<int>
    {
        public int x, y, z, w;
        public uint Dimensionality { get { return 4; } }

        #region Constructors

        public Int4(int x)
        {
            this.x = x;
            y = 0;
            z = 0;
            w = 0;
        }

        public Int4(Int fromInt)
        {
            x = fromInt.x;
            y = 0;
            z = 0;
            w = 0;
        }

        public Int4(int x, int y)
        {
            this.x = x;
            this.y = y;
            z = 0;
            w = 0;
        }

        public Int4(Int2 fromInt)
        {
            x = fromInt.x;
            y = fromInt.y;
            z = 0;
            w = 0;
        }

        public Int4(int x, int y, int z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            w = 0;
        }

        public Int4(Int3 fromInt)
        {
            x = fromInt.x;
            y = fromInt.y;
            z = fromInt.z;
            w = 0;
        }
        
        public Int4(int x, int y, int z, int w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }
 
        public Int4(Int4 fromInt)
        {
            x = fromInt.x;
            y = fromInt.y;
            z = fromInt.z;
            w = fromInt.w;
        }

        public static Int4 zero { get { return new Int4(0, 0, 0, 0); } }
        public static Int4 one { get { return new Int4(1, 1, 1, 1); } }

        #endregion

        #region Operators

        public Int4 Add(Int4 other) { return this + other; }
        public static Int4 operator +(Int4 a) { return new Int4(Math.Abs(a.x), Math.Abs(a.y), Math.Abs(a.z), Math.Abs(a.w)); }
        public static Int4 operator +(Int4 a, int b) { return new Int4(a.x + b, a.y + b, a.z + b, a.w + b); }
        public static Int4 operator +(Int4 a, Int2 b) { return new Int4(a.x + b.x, a.y + b.y, a.z, a.w); }
        public static Int4 operator +(Int4 a, Int3 b) { return new Int4(a.x + b.x, a.y + b.y, a.z + b.z, a.w); }
        public static Int4 operator +(Int4 a, Int4 b) { return new Int4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
        public static Int4 operator ++(Int4 a) { return new Int4(a.x + 1, a.y + 1, a.z + 1, a.w +1); }

        public Int4 Subtract(Int4 other) { return this - other; }
        public static Int4 operator -(Int4 a) { return new Int4(-a.x, -a.y, -a.z, -a.w); }
        public static Int4 operator -(Int4 a, int b) { return new Int4(a.x - b, a.y - b, a.z - b, a.w - b); }
        public static Int4 operator -(Int4 a, Int2 b) { return new Int4(a.x - b.x, a.y - b.y, a.z, a.w); }
        public static Int4 operator -(Int4 a, Int3 b) { return new Int4(a.x - b.x, a.y - b.y, a.z - b.z, a.w); }
        public static Int4 operator -(Int4 a, Int4 b) { return new Int4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
        public static Int4 operator --(Int4 a) { return new Int4(a.x - 1, a.y - 1, a.z - 1, a.w - 1); }

        public Int4 Multiply(Int4 other) { return this * other; }
        public static Int4 operator *(Int4 a, int b) { return new Int4(a.x * b, a.y * b, a.z * b, a.w *b); }
        public static Int4 operator *(Int4 a, Int2 b) { return new Int4(a.x * b.x, a.y * b.y, a.z, a.w); }
        public static Int4 operator *(Int4 a, Int3 b) { return new Int4(a.x * b.x, a.y * b.y, a.z * b.z, a.w); }
        public static Int4 operator *(Int4 a, Int4 b) { return new Int4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }

        public Int4 Divide(Int4 other) { return this / other; }
        public static Int4 operator /(Int4 a, int b) { return new Int4(a.x / b, a.y / b, a.z / b, a.w / b); }
        public static Int4 operator /(Int4 a, Int2 b) { return new Int4(a.x / b.x, a.y / b.y, a.z, a.w); }
        public static Int4 operator /(Int4 a, Int3 b) { return new Int4(a.x / b.x, a.y / b.y, a.z / b.z, a.w); }
        public static Int4 operator /(Int4 a, Int4 b) { return new Int4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }

        public static Int4 operator %(Int4 a, int b) { return new Int4(a.x % b, a.y % b, a.z % b, a.w % b); }
        public static Int4 operator %(Int4 a, Int2 b) { return new Int4(a.x % b.x, a.y % b.y, a.z, a.w); }
        public static Int4 operator %(Int4 a, Int3 b) { return new Int4(a.x % b.x, a.y % b.y, a.z % b.z, a.w); }
        public static Int4 operator %(Int4 a, Int4 b) { return new Int4(a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w); }

        public static bool operator ==(Int4 a, Int4 b)
        {
            return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
        }

        public static bool operator !=(Int4 a, Int4 b)
        {
            return !(a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
        }

        public override bool Equals(object obj)
        {
            return obj is Int4 newInt && Equals(newInt);
        }

        public bool Equals(Int4 other)
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


        public bool TryConvert(object obj, out Int4 value)
        {
            if (obj is Int4 i) { value = i; return true; }
            //else if (obj is UnityEngine.Vector4Int v2) { value = new Int2(v2.x, v2.y); return true; }
            value = Int4.zero;
            return false;
        }
        #endregion

        #region Methods

        public bool Compare(Int4 other, ValueComparison comparison = ValueComparison.EqualTo)
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

        public Int4 Abs() { return new Int4(Int.Abs(x), Int.Abs(y), Int.Abs(z), Int.Abs(w)); }
        #endregion

        #region IDimesional Implementation

        public void SetDimension(int dimension, int value)
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

        public int GetDimension(int dimension)
        {
            switch (dimension)
            {
                case 0: return x;
                case 1: return y;
                case 2: return z;
                case 3: return w;
                default: return 0;
            }
        }

        #endregion
    }
}
