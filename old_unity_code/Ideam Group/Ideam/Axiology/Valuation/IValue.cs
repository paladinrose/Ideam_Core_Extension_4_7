using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Axiology.Valuation
{
    public enum ValueComparison
    {
        EqualTo,
        NotEqual,
        LessThan,
        LessThanOrEqualTo,
        GreaterThan,
        GreaterThanOrEqualTo
    }

    public interface IValue<TValue> : IEquatable<TValue>
    {
        TValue Add(TValue other);
        TValue Subtract(TValue other);
        TValue Multiply(TValue other);
        TValue Divide(TValue other);
        TValue Abs();

        bool Compare(TValue value, ValueComparison comparison);

        bool TryConvert(object obj, out TValue value);
    }
}
