using System;

using Ideam.Axiology.Valuation;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Bool : IValue<Bool>, IEquatable<bool>
    {
        public bool b;
        public Bool(bool b) { this.b = b; }
        public Bool(Bool b) { this.b = b.b; }

        public static bool operator ==(Bool a, Bool b)
        {
            return a.b == b.b;
        }

        public static bool operator !=(Bool a, Bool b)
        {
            return !(a.b == b.b);
        }

        public override bool Equals(object obj)
        {
            return obj is Bool newFloat && Equals(newFloat);
        }

        public bool Equals(Bool other)
        {
            return b == other.b;
        }
        public bool Equals(bool other)
        {
            return b == other;
        }

        public static implicit operator bool(Bool b)
        {
            return b.b;
        }
        public static implicit operator Bool(bool b)
        {
            return new Bool(b);
        }
        public Bool Add(Bool other)
        {
            return new Bool(b && other.b);
        }

        public Bool Subtract(Bool other)
        {
            return new Bool(b != other.b);
        }

        public Bool Multiply(Bool other)
        {
            return new Bool(b && other.b);
        }

        public Bool Divide(Bool other)
        {
            return new Bool(b && other.b);
        }

        public Bool Abs()
        {
            throw new NotImplementedException();
        }

        public bool Compare(Bool value, ValueComparison comparison)
        {
            throw new NotImplementedException();
        }

        public override int GetHashCode()
        {
            return -499555667 + b.GetHashCode();
        }

        public bool TryConvert(object obj, out Bool value)
        {
            bool b = obj != null;
            value = b;
            return b;
        }
    }
}
