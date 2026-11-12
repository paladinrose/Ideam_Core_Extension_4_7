using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;

namespace Ideam.Science.Natural.Physics
{
    public interface IFundamentalField
    {
        /// <summary>
        /// Represents the zero-point energy state of the field.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        T FieldConstant<T>() where T:IValue<T>;

        T Infinimum<T>() where T : IValue<T>;

        T Supremum<T>() where T : IValue<T>;
    }
}
