
namespace Ideam.Science.Formal.Mathematics.Scalars
{
    public class ConstantScalarField : BaseScalarField
    {
        private readonly float _constant;

        public ConstantScalarField(float constant)
        {
            _constant = constant;
        }

        public override float Sample(Float2 position)
        {
            return _constant;
        }
    }

}
