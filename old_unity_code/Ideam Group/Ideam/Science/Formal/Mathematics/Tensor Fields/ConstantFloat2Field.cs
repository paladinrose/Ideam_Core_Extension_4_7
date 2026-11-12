namespace Ideam.Science.Formal.Mathematics.VectorFields
{
    public class ConstantFloat2Field : IFloat2Field
    {
        private readonly Float2 _value;

        public ConstantFloat2Field(Float2 value)
        {
            _value = value;
        }

        public Float2 Sample(Float2 position)
        {
            return _value;
        }
    }


}
