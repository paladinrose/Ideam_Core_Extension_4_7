namespace Ideam.Science.Formal.Mathematics.Scalars
{
    public class MultiplierScalarField : BaseScalarField
    {
        private readonly BaseScalarField _baseField;
        private readonly float _scale;

        public MultiplierScalarField(BaseScalarField baseField, float scale)
        {
            _baseField = baseField;
            _scale = scale;
        }

        public override float Sample(Float2 position)
        {
            return _baseField.Sample(position) * _scale;
        }
    }

}
