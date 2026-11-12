using Ideam.Science.Formal.Mathematics.Scalars;

namespace Ideam.Science.Formal.Mathematics.VectorFields
{
    public class GradientFloat2Field : IFloat2Field
    {
        private readonly BaseScalarField _scalar;

        public GradientFloat2Field(BaseScalarField scalar)
        {
           
            _scalar = scalar;
        }

        
        public Float2 Sample(Float2 position)
        {
            var v = _scalar.Sample(position);
            var x = _scalar.Sample(new Float2(position.x + 1, position.y));
            var y = _scalar.Sample(new Float2(position.x, position.y + 1));

            return new Float2(
                v - x,
                v - y
            );
        }
    }


}
