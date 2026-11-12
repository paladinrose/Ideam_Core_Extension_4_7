using System;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;
namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Int3 : IValue<Int3>, IDimensional<int>
    {
        public int x, y, z;
        public uint Dimensionality { get { return 3; } }

        #region Constructors

        public Int3(int x)
        {
            this.x = x;
            y = 0;
            z = 0;
        }

        public Int3(Int fromInt)
        {
            x = fromInt.x;
            y = 0;
            z = 0;
        }

        public Int3(int x, int y)
        {
            this.x = x;
            this.y = y;
            z = 0;
        }

        public Int3(Int2 fromInt)
        {
            x = fromInt.x;
            y = fromInt.y;
            z = 0;
        }
        
        public Int3(int x, int y, int z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public Int3(Int3 fromInt)
        {
            x = fromInt.x;
            y = fromInt.y;
            z = fromInt.z;
        }

        public static Int3 zero { get { return new Int3(0, 0, 0); } }
        public static Int3 one { get { return new Int3(1, 1, 1); } }


        #endregion

        #region Operators

        public Int3 Add(Int3 other) { return this + other; }
        public static Int3 operator +(Int3 a) { return new Int3(Math.Abs(a.x), Math.Abs(a.y), Math.Abs(a.z)); }
        public static Int3 operator +(Int3 a, int b) { return new Int3(a.x + b, a.y + b, a.z + b); }
        public static Int3 operator +(Int3 a, Int2 b) { return new Int3(a.x + b.x, a.y + b.y, a.z); }
        public static Int3 operator +(Int3 a, Int3 b) { return new Int3(a.x + b.x, a.y + b.y, a.z + b.z); }
        public static Int3 operator ++(Int3 a) { return new Int3(a.x + 1, a.y + 1, a.z + 1); }

        public Int3 Subtract(Int3 other) { return this - other; }
        public static Int3 operator -(Int3 a) { return new Int3(-a.x, -a.y, -a.z); }
        public static Int3 operator -(Int3 a, int b) { return new Int3(a.x - b, a.y - b, a.z - b); }
        public static Int3 operator -(Int3 a, Int2 b) { return new Int3(a.x - b.x, a.y - b.y, a.z); }
        public static Int3 operator -(Int3 a, Int3 b) { return new Int3(a.x - b.x, a.y - b.y, a.z - b.z); }
        public static Int3 operator --(Int3 a) { return new Int3(a.x - 1, a.y - 1, a.z - 1); }

        public Int3 Multiply(Int3 other) { return this * other; }
        public static Int3 operator *(Int3 a, int b) { return new Int3(a.x * b, a.y * b, a.z * b); }
        public static Int3 operator *(Int3 a, Int2 b) { return new Int3(a.x * b.x, a.y * b.y, a.z); }
        public static Int3 operator *(Int3 a, Int3 b) { return new Int3(a.x * b.x, a.y * b.y, a.z * b.z); }

        public Int3 Divide(Int3 other) { return this / other; }
        public static Int3 operator /(Int3 a, int b) { return new Int3(a.x / b, a.y / b, a.z / b); }
        public static Int3 operator /(Int3 a, Int2 b) { return new Int3(a.x / b.x, a.y / b.y, a.z); }
        public static Int3 operator /(Int3 a, Int3 b) { return new Int3(a.x / b.x, a.y / b.y, a.z / b.z); }

        public static Int3 operator %(Int3 a, int b) { return new Int3(a.x % b, a.y % b, a.z % b); }
        public static Int3 operator %(Int3 a, Int2 b) { return new Int3(a.x % b.x, a.y % b.y, a.z); }
        public static Int3 operator %(Int3 a, Int3 b) { return new Int3(a.x % b.x, a.y % b.y, a.z % b.z); }

        public static bool operator ==(Int3 a, Int3 b)
        {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }

        public static bool operator !=(Int3 a, Int3 b)
        {
            return !(a.x == b.x && a.y == b.y && a.z == b.z);
        }

        public override bool Equals(object obj)
        {
            return obj is Int3 newInt && Equals(newInt);
        }

        public bool Equals(Int3 other)
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

        public bool TryConvert(object obj, out Int3 value)
        {
            if (obj is Int3 i) { value = i; return true; }
            else if (obj is UnityEngine.Vector3Int v3) { value = new Int3(v3.x, v3.y, v3.z); return true; }
            value = Int3.zero;
            return false;
        }

        #endregion

        #region Methods

        public bool Compare(Int3 other, ValueComparison comparison = ValueComparison.EqualTo)
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

        public Int3 Abs() { return new Int3(Int.Abs(x), Int.Abs(y), Int.Abs(z)); }
        #endregion

        #region IDimesional Implementation

        public void SetDimension(int dimension, int value)
        {
            switch (dimension)
            {
                case 0: x = value; break;
                case 1: y = value; break;
                case 2: z = value; break;
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
                default: return 0;
            }
        }

        #endregion
    }
}
