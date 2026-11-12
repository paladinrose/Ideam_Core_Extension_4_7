namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public class GridlineTensorField : ITensorField
    {
        private readonly Tensor _basis;

        public GridlineTensorField(float angle, float length)
        {
            _basis = Tensor.FromRTheta(length, angle);
        }

        public void Sample(ref Float2 position, out Tensor result)
        {
            result = _basis;
        }

    }
}
