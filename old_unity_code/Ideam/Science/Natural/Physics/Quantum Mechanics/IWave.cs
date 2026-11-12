using Ideam.Science.Formal.Mathematics;

namespace Ideam.Science.Natural.Physics.QuantumMechanics
{
    public interface IWave 
    {
        T[] GetWaveElements<T>() where T : IQuanta;
        void Collapse();
    }
}