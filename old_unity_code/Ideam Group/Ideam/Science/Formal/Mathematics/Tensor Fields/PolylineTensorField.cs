using System;

using Ideam.Science.Formal.Mathematics.Tensors.Tracing;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public class PolylineTensorField : ITensorField
    {
        private readonly Float2[] _points;
        private readonly float _decay;

        public PolylineTensorField(Float2[] points, float decay)
        {
            _points = points;
            _decay = decay;
        }

        public void Sample(ref Float2 position, out Tensor result)
        {
            result = new Tensor(0, 0);

            for (var i = 0; i < _points.Length - 1; i++)
            {
                var start = _points[i];
                var end = _points[i + 1];
                var dir = (end - start).Normalize();

                var angle = Math.Atan2(dir.y, dir.x) + Math.PI/2f;
                var tensor = Tensor.Normalize(Tensor.FromRTheta(1, angle));

                var dist = Math.Abs(new LineSegment2(start, end).DistanceToPoint(position));
                var decay = PointDistanceDecayTensorField.DistanceDecay(dist * dist, _decay);

                result += decay * tensor;
            }
        }
    }
}
