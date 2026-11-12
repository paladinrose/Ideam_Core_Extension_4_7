namespace Ideam.Science.Formal.Mathematics.VectorFields
{
    public interface IFloat2Field
    {
        Float2 Sample(Float2 position);

        
    }

    internal static class InternalIFloat2FieldExtensions
    {
        public static Float2 TraceVectorField(this IFloat2Field field, Float2 position, Float2 previous, float maxSegmentLength)
        {
            
            var first = previous;

            //Extended naieve tracing (accumulate naieve traces, stop once we hit sample OR length limit)
            float lengthSum = 0;
            var result = Float2.zero;
            for (var i = 0; i < 10 && lengthSum < maxSegmentLength; i++)
            {
                var d = field.SingleTraceVectorField(position, previous);
                var l = d.Magnitude();
                lengthSum += d.Magnitude();
                result += d;
                previous = d;

                //escape if the curvature is too much
                if (Float2.Dot(first, d / l) < 0.9961f)
                    break;
            }

            return result;
        }

        private static Float2 SingleTraceVectorField(this IFloat2Field field, Float2 position, Float2 previous)
        {
            
            //Naieve tracing
            //return field.CorrectedSample(position, previous);

            //RK4
            var k1 = field.CorrectedSample(position, previous);
            var k2 = field.CorrectedSample(position + k1 / 2f, previous);
            var k3 = field.CorrectedSample(position + k2 / 2f, previous);
            var k4 = field.CorrectedSample(position + k3, previous);
            return CorrectVectorDirection(k1 / 6f + k2 / 3f + k3 / 3f + k4 / 6f, previous);
        }

        private static Float2 CorrectedSample(this IFloat2Field field, Float2 position, Float2 baseDirection)
        {
            
            return CorrectVectorDirection(field.Sample(position), baseDirection);
        }

        private static Float2 CorrectVectorDirection(Float2 v, Float2 previous)
        {
            if (previous == Float2.zero || Float2.Dot(previous, v) >= 0)
                return v;
            return -v;
        }
    }
}
