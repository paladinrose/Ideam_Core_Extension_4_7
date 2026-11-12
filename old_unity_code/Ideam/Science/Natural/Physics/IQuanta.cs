using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;
namespace Ideam.Science.Natural.Physics
{
    /// <summary>
    /// A Quanta of space represents the measured energy states of multiple fundamental fields
    /// in one point location.
    /// </summary>
    public interface IQuanta
    {
        /// <summary>
        /// Particles are ordinarily the only fundamental things, but the system can naively treat
        /// more complex objects as fundamental by defining this as true.
        /// </summary>
        bool IsFundamental { get; }

        /// <summary>
        /// Returns the actual values across all fundamental fields for this quanta. In a real
        /// quantum system, this is most likely impossible to know.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        T[] ActualValues<T>() where T : IValue<T>;

        /// <summary>
        /// Represents the measurement of a property of this quanta. In a real quantum system, 
        /// this involves increasing uncertainty in a DIFFERENT property, but this is not assumed in code. 
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <typeparam name="TValue"></typeparam>
        /// <returns></returns>
        T Measure<T, TField, TPointer>(TField field, TPointer pointerState)
            where T:IValue<T> where TField:IFundamentalField where TPointer:IPointerState;
    }

    /// <summary>
    /// Represents one or more environmental or systemic states that cause the measuremant of one or more quantum states.
    /// </summary>
    public interface IPointerState
    {
        //The Fundamental Field that this state effects.
        int FieldID { get; }

    }
}
