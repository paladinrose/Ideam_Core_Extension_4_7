using Ideam.Axiology.Valuation;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Natural.Cosmology;

namespace Ideam.Science.Natural.Physics.QuantumMechanics
{
    public interface IHilbertSpace
    {
        /// <summary>
        /// The universe defines the dimensionality, and fundamental fields of the hilbert space.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        T Universe<T>() where T : IUniverse;

        T Bounds<T, TValue>() where T : IBoundary<T, TValue> where TValue : IValue<TValue>;

        /// <summary>
        /// Returns the limit that fundamental values(id) converge towards.  
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="id"></param>
        /// <returns></returns>
        T GetFieldLimit<T>(int id) where T : IValue<T>;
      
        T Sample<T, TID>(TID location, int fieldID) where T : IQuanta where TID : IValue<TID>;
    }
}