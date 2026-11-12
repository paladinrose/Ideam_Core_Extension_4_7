using Ideam.Science.Formal.Mathematics.VectorFields;

namespace Ideam.Science.Formal.Mathematics.Scalars
{
    public abstract class BaseScalarField
    {
        public abstract float Sample(Float2 position);

        public IFloat2Field Gradient()
        {
            
            return new GradientFloat2Field(this);
        }

        public static BaseScalarField operator *(BaseScalarField field, float value)
        {
            
            return new MultiplierScalarField(field, value);
        }
    }

}
