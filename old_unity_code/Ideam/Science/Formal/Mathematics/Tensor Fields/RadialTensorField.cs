using System;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public class RadialTensorField : ITensorField
    {
        private readonly Float2 _center;

        public RadialTensorField(Float2 center)
        {
            _center = center;
        }

        public void Sample(ref Float2 position, out Tensor result)
        {
            result = Tensor.FromXY(position - _center);
        }

       
    }
}
