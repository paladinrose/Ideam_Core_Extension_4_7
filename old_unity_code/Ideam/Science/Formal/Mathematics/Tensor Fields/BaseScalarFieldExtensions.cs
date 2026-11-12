namespace Ideam.Science.Formal.Mathematics.Scalars
{
    public static class BaseScalarFieldExtensions
    {
        public static float SafeSample(this BaseScalarField field, Float2 position)
        {
            return field == null ? 0 : field.Sample(position);
        }
    }

}
