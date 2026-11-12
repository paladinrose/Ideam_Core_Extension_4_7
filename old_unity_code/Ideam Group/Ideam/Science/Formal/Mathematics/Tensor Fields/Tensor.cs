using System;
using System.Numerics;
using System.Collections;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public struct Tensor
    {
        // A tensor is a 2x2 symmetric and traceless matrix of the form
        // R * | cos(2theta)  sin(2theta) |  = | a b |
        //     | sin(2theta) -cos(2theta) |    | _ _ |
        // where R >= 0 and theta is [0, 2pi)

        public readonly double A;
        public readonly double B;

        public Tensor(double a, double b)
        {
            A = a;
            B = b;
        }

        public static Tensor FromRTheta(double r, double theta)
        {
            return new Tensor(r * Math.Cos(2 * theta), r * Math.Sin(2 * theta));
        }

        public static Tensor FromXY(Float2 xy)
        {
            var xy2 = -2 * xy.x * xy.y;
            var diffSquares = xy.y * xy.y - xy.x * xy.x;
            return Normalize(new Tensor(diffSquares, xy2));
        }

        public static Tensor Normalize(Tensor tensor)
        {
            var l = Math.Sqrt(tensor.A * tensor.A + tensor.B * tensor.B);
            if (Math.Abs(l) < float.Epsilon)
                return new Tensor(0, 0);

            return new Tensor(tensor.A / l, tensor.B / l);

        }

        public static Tensor operator +(Tensor left, Tensor right)
        {
            return new Tensor(left.A + right.A, left.B + right.B);
        }

        public static Tensor operator *(double left, Tensor right)
        {
            return new Tensor(left * right.A, left * right.B);
        }

        //Eigen calculation based on http://www.math.harvard.edu/archive/21b_fall_04/exhibits/2dmatrices/index.html
        public void EigenValues(out double e1, out double e2)
        {
            var eval = Math.Sqrt(A * A + B * B);

            e1 = eval;
            e2 = -eval;
        }

        public void EigenVectors(out Float2 major, out Float2 minor)
        {
            if (Math.Abs(B) < 0.0000001f)
            {
                if (Math.Abs(A) < 0.0000001f)
                {
                    major = Float2.zero;
                    minor = Float2.zero;
                }
                else
                {
                    major = new Float2(1, 0);
                    minor = new Float2(0, 1);
                }
            }
            else
            {
                double e1, e2;
                EigenValues(out e1, out e2);

                major = new Float2((float)B, (float)(e1 - A));
                minor = new Float2((float)B, (float)(e2 - A));
            }
        }
    }
}
