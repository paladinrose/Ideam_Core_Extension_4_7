using System;
using System.Linq;
using System.Threading.Tasks;

using Ideam.Science.Formal.Mathematics.VectorFields;
using Ideam.Science.Formal.Mathematics.Tensors;

namespace Ideam.Science.Formal.Mathematics.EiginVectors
{
    public class ResampleAndRescaleEigenField : IEigenField
    {
        private readonly IFloat2Field _major;
        public IFloat2Field MajorEigenVectors { get { return _major; } }

        private readonly IFloat2Field _minor;
        public IFloat2Field MinorEigenVectors { get { return _minor; } }

        private readonly Float2[,] _majorEigenVectors;
        private readonly float _xLength;
        private readonly float _yLength;

        private readonly Float2 _size;
        private readonly Float2 _min;

        private readonly bool _zeroSize;

        private ResampleAndRescaleEigenField(Float2[,] major, Float2 min, Float2 max)
        {
            _min = min;
            _size = max - min;

            _zeroSize = (Math.Abs(_size.x) < float.Epsilon || Math.Abs(_size.y) < float.Epsilon);

            if (_zeroSize)
                return;

            _majorEigenVectors = major;

            _xLength = major.GetLength(0) - 1;
            _yLength = major.GetLength(1) - 1;

            _minor = new EigenAccessor(true, this);
            _major = new EigenAccessor(false, this);
        }

        private void Sample(bool major, ref Float2 position, out Float2 result)
        {
            //If we've already established that this array is zero size, return nothing
            if (_zeroSize)
            {
                result = new Float2(0, 0);
                return;
            }

            //Work out array position from vector field position
            var p = ((position - _min) / _size);
            var ij = p * new Float2(_xLength, _yLength);
            var i = (int)Math.Clamp(ij.x, 0, _xLength);
            var j = (int)Math.Clamp(ij.y, 0, _yLength);

            //Result from array (no interpolation)
            result = _majorEigenVectors[i, j];

            //Flip it from major to minor as necessary
            if (!major)
                result = new Float2(-result.y, result.x);
        }

        public static IEigenField Create(ITensorField baseField, Float2 min, Float2 max, uint resolution)
        {
            var major = new Float2[resolution + 1, resolution + 1];

            Parallel.For(0, resolution + 1, i =>
                Parallel.For(0, resolution + 1, j => {
                    var p = new Float2(i / (float)resolution, j / (float)resolution) + (min / new Float2(resolution, resolution));

                    Tensor t;
                    baseField.Sample(ref p, out t);

                    Float2 majEigen, minEigen;
                    t.EigenVectors(out majEigen, out minEigen);

                    major[i, j] = majEigen;
                })
            );

            return new ResampleAndRescaleEigenField(major, min, max);
        }

        private class EigenAccessor
            : IFloat2Field
        {
            private readonly bool _major;
            private readonly ResampleAndRescaleEigenField _field;

            public EigenAccessor(bool major, ResampleAndRescaleEigenField field)
            {
                _major = major;
                _field = field;
            }


            public Float2 Sample(Float2 position)
            {
                Float2 result;
                _field.Sample(_major, ref position, out result);
                return result;
            }
        }
    }

}
