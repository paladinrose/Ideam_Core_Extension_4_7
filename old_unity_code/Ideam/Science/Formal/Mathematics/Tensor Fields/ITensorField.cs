using System.Numerics;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public interface ITensorField
    {
        void Sample(ref Float2 position, out Tensor result);
    }

}
