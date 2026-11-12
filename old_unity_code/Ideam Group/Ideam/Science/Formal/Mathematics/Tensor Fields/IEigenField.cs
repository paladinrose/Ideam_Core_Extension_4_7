using Ideam.Science.Formal.Mathematics.VectorFields;

namespace Ideam.Science.Formal.Mathematics.EiginVectors
{
    public interface IEigenField
    {
        IFloat2Field MajorEigenVectors { get; }

        IFloat2Field MinorEigenVectors { get; }
    }

}
