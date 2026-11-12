namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public class ConstantTensorField : ITensorField
    {
        private readonly Tensor _value;

        public ConstantTensorField(Tensor value)
        {
            _value = value;
        }

        public void Sample(ref Float2 position, out Tensor result)
        {
            result = _value;
        }
    }
}
