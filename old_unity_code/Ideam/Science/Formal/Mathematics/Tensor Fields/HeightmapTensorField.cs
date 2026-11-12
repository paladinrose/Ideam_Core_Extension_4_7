using System;

using Ideam.Science.Formal.Mathematics.VectorFields;
using Ideam.Science.Formal.Mathematics.Scalars;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public class HeightmapTensorField
        : ITensorField
    {
        private readonly IFloat2Field _gradient;

        public HeightmapTensorField(BaseScalarField height)
        {
            
            _gradient = new GradientFloat2Field(height);
        }

        public void Sample(ref Float2 position, out Tensor result)
        {
            var grad = _gradient.Sample(position);

            var theta = (float)Math.Atan2(grad.y, grad.x) + Math.PI/2f;
            var r = (float)Math.Sqrt(grad.x * grad.x + grad.y * grad.y);

            result = Tensor.Normalize(Tensor.FromRTheta(r, theta));
        }
    }
}
