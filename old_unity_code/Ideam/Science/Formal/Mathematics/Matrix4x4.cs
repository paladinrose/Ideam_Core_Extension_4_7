using System;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Matrix4x4 : IEquatable<Matrix4x4>
    {
        
        public float M11, M21, M31, M41, 
                     M12, M22, M32, M42,
                     M13, M23, M33, M43,
                     M14, M24, M34, M44;
        
        

        public Matrix4x4(Float4 column0, Float4 column1, Float4 column2, Float4 column3)
        {
            this.M11 = column0.x; this.M12 = column1.x; this.M13 = column2.x; this.M14 = column3.x;
            this.M21 = column0.y; this.M22 = column1.y; this.M23 = column2.y; this.M24 = column3.y;
            this.M31 = column0.z; this.M32 = column1.z; this.M33 = column2.z; this.M34 = column3.z;
            this.M41 = column0.w; this.M42 = column1.w; this.M43 = column2.w; this.M44 = column3.w;
        }

        // Access element at [row, column].
        public float this[int row, int column]
        {
            get
            {
                return this[row + column * 4];
            }

            set
            {
                this[row + column * 4] = value;
            }
        }

        // Access element at sequential index (0..15 inclusive).
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                    case 0: return M11;
                    case 1: return M21;
                    case 2: return M31;
                    case 3: return M41;
                    case 4: return M12;
                    case 5: return M22;
                    case 6: return M32;
                    case 7: return M42;
                    case 8: return M13;
                    case 9: return M23;
                    case 10: return M33;
                    case 11: return M43;
                    case 12: return M14;
                    case 13: return M24;
                    case 14: return M34;
                    case 15: return M44;
                    default:
                        throw new IndexOutOfRangeException("Invalid matrix index!");
                }
            }

            set
            {
                switch (index)
                {
                    case 0: M11 = value; break;
                    case 1: M21 = value; break;
                    case 2: M31 = value; break;
                    case 3: M41 = value; break;
                    case 4: M12 = value; break;
                    case 5: M22 = value; break;
                    case 6: M32 = value; break;
                    case 7: M42 = value; break;
                    case 8: M13 = value; break;
                    case 9: M23 = value; break;
                    case 10: M33 = value; break;
                    case 11: M43 = value; break;
                    case 12: M14 = value; break;
                    case 13: M24 = value; break;
                    case 14: M34 = value; break;
                    case 15: M44 = value; break;

                    default:
                        throw new IndexOutOfRangeException("Invalid matrix index!");
                }
            }
        }

        // used to allow Matrix4x4s to be used as keys in hash tables
        public override int GetHashCode()
        {
            return GetColumn(0).GetHashCode() ^ (GetColumn(1).GetHashCode() << 2) ^ (GetColumn(2).GetHashCode() >> 2) ^ (GetColumn(3).GetHashCode() >> 1);
        }

        // also required for being able to use Matrix4x4s as keys in hash tables
        public override bool Equals(object other)
        {
            if (!(other is Matrix4x4)) return false;

            return Equals((Matrix4x4)other);
        }

        public bool Equals(Matrix4x4 other)
        {
            return GetColumn(0).Equals(other.GetColumn(0))
                && GetColumn(1).Equals(other.GetColumn(1))
                && GetColumn(2).Equals(other.GetColumn(2))
                && GetColumn(3).Equals(other.GetColumn(3));
        }

        public static Matrix4x4 operator *(Matrix4x4 lhs, Matrix4x4 rhs)
        {
            Matrix4x4 res;
            res.M11 = lhs.M11 * rhs.M11 + lhs.M12 * rhs.M21 + lhs.M13 * rhs.M31 + lhs.M14 * rhs.M41;
            res.M12 = lhs.M11 * rhs.M12 + lhs.M12 * rhs.M22 + lhs.M13 * rhs.M32 + lhs.M14 * rhs.M42;
            res.M13 = lhs.M11 * rhs.M13 + lhs.M12 * rhs.M23 + lhs.M13 * rhs.M33 + lhs.M14 * rhs.M43;
            res.M14 = lhs.M11 * rhs.M14 + lhs.M12 * rhs.M24 + lhs.M13 * rhs.M34 + lhs.M14 * rhs.M44;

            res.M21 = lhs.M21 * rhs.M11 + lhs.M22 * rhs.M21 + lhs.M23 * rhs.M31 + lhs.M24 * rhs.M41;
            res.M22 = lhs.M21 * rhs.M12 + lhs.M22 * rhs.M22 + lhs.M23 * rhs.M32 + lhs.M24 * rhs.M42;
            res.M23 = lhs.M21 * rhs.M13 + lhs.M22 * rhs.M23 + lhs.M23 * rhs.M33 + lhs.M24 * rhs.M43;
            res.M24 = lhs.M21 * rhs.M14 + lhs.M22 * rhs.M24 + lhs.M23 * rhs.M34 + lhs.M24 * rhs.M44;

            res.M31 = lhs.M31 * rhs.M11 + lhs.M32 * rhs.M21 + lhs.M33 * rhs.M31 + lhs.M34 * rhs.M41;
            res.M32 = lhs.M31 * rhs.M12 + lhs.M32 * rhs.M22 + lhs.M33 * rhs.M32 + lhs.M34 * rhs.M42;
            res.M33 = lhs.M31 * rhs.M13 + lhs.M32 * rhs.M23 + lhs.M33 * rhs.M33 + lhs.M34 * rhs.M43;
            res.M34 = lhs.M31 * rhs.M14 + lhs.M32 * rhs.M24 + lhs.M33 * rhs.M34 + lhs.M34 * rhs.M44;

            res.M41 = lhs.M41 * rhs.M11 + lhs.M42 * rhs.M21 + lhs.M43 * rhs.M31 + lhs.M44 * rhs.M41;
            res.M42 = lhs.M41 * rhs.M12 + lhs.M42 * rhs.M22 + lhs.M43 * rhs.M32 + lhs.M44 * rhs.M42;
            res.M43 = lhs.M41 * rhs.M13 + lhs.M42 * rhs.M23 + lhs.M43 * rhs.M33 + lhs.M44 * rhs.M43;
            res.M44 = lhs.M41 * rhs.M14 + lhs.M42 * rhs.M24 + lhs.M43 * rhs.M34 + lhs.M44 * rhs.M44;

            return res;
        }

        public static Float4 operator *(Matrix4x4 lhs, Float4 vector)
        {
            Float4 res;
            res.x = lhs.M11 * vector.x + lhs.M12 * vector.y + lhs.M13 * vector.z + lhs.M14 * vector.w;
            res.y = lhs.M21 * vector.x + lhs.M22 * vector.y + lhs.M23 * vector.z + lhs.M24 * vector.w;
            res.z = lhs.M31 * vector.x + lhs.M32 * vector.y + lhs.M33 * vector.z + lhs.M34 * vector.w;
            res.w = lhs.M41 * vector.x + lhs.M42 * vector.y + lhs.M43 * vector.z + lhs.M44 * vector.w;
            return res;
        }

        public static bool operator ==(Matrix4x4 lhs, Matrix4x4 rhs)
        {
            return lhs.GetColumn(0) == rhs.GetColumn(0)
                && lhs.GetColumn(1) == rhs.GetColumn(1)
                && lhs.GetColumn(2) == rhs.GetColumn(2)
                && lhs.GetColumn(3) == rhs.GetColumn(3);
        }

        public static bool operator !=(Matrix4x4 lhs, Matrix4x4 rhs)
        {
            return !(lhs == rhs);
        }

        public Float4 GetColumn(int index)
        {
            switch (index)
            {
                case 0: return new Float4(M11, M21, M31, M41);
                case 1: return new Float4(M12, M22, M32, M42);
                case 2: return new Float4(M13, M23, M33, M43);
                case 3: return new Float4(M14, M24, M34, M44);
                default:
                    throw new IndexOutOfRangeException("Invalid column index!");
            }
        }

        public Float4 GetRow(int index)
        {
            switch (index)
            {
                case 0: return new Float4(M11, M12, M13, M14);
                case 1: return new Float4(M21, M22, M23, M24);
                case 2: return new Float4(M31, M32, M33, M34);
                case 3: return new Float4(M41, M42, M43, M44);
                default:
                    throw new IndexOutOfRangeException("Invalid row index!");
            }
        }

        public void SetColumn(int index, Float4 column)
        {
            this[0, index] = column.x;
            this[1, index] = column.y;
            this[2, index] = column.z;
            this[3, index] = column.w;
        }

        public void SetRow(int index, Float4 row)
        {
            this[index, 0] = row.x;
            this[index, 1] = row.y;
            this[index, 2] = row.z;
            this[index, 3] = row.w;
        }

        public float Determinant()
        {

            Matrix3x3 ma = new Matrix3x3(M22, M32, M42,
                                         M23, M33, M43,
                                         M24, M34, M44),

                      mb = new Matrix3x3(M12, M32, M42,
                                         M13, M33, M43,
                                         M14, M34, M44),

                      mc = new Matrix3x3(M12, M22, M42,
                                         M13, M23, M43,
                                         M13, M24, M44),

                      md = new Matrix3x3(M12, M22, M32,
                                         M13, M23, M33,
                                         M14, M24, M34);
            float a = M11 * ma.Determinant(),
                  b = M21 * mb.Determinant(),
                  c = M31 * mc.Determinant(),
                  d = M41 * md.Determinant();
            return a - b + c - d;
        }

        public Matrix4x4 Inverse()
        {
            return Matrix4x4.identity;
        }

        public Float3 MultiplyPoint(Float3 point)
        {
            Float3 res;
            float w;
            res.x = this.M11 * point.x + this.M12 * point.y + this.M13 * point.z + this.M14;
            res.y = this.M21 * point.x + this.M22 * point.y + this.M23 * point.z + this.M24;
            res.z = this.M31 * point.x + this.M32 * point.y + this.M33 * point.z + this.M34;
            w = this.M41 * point.x + this.M42 * point.y + this.M43 * point.z + this.M44;

            w = 1F / w;
            res.x *= w;
            res.y *= w;
            res.z *= w;
            return res;
        }

        // Transforms a position by this matrix, without a perspective divide. (fast)
        public Float3 MultiplyPoint3x4(Float3 point)
        {
            Float3 res;
            res.x = this.M11 * point.x + this.M12 * point.y + this.M13 * point.z + this.M14;
            res.y = this.M21 * point.x + this.M22 * point.y + this.M23 * point.z + this.M24;
            res.z = this.M31 * point.x + this.M32 * point.y + this.M33 * point.z + this.M34;
            return res;
        }

        // Transforms a direction by this matrix.
        public Float3 MultiplyFloat(Float3 vector)
        {
            Float3 res;
            res.x = this.M11 * vector.x + this.M12 * vector.y + this.M13 * vector.z;
            res.y = this.M21 * vector.x + this.M22 * vector.y + this.M23 * vector.z;
            res.z = this.M31 * vector.x + this.M32 * vector.y + this.M33 * vector.z;
            return res;
        }

        /*
        // Transforms a plane by this matrix.
        public Plane TransformPlane(Plane plane)
        {
            var ittrans = this.inverse;

            float x = plane.normal.x, y = plane.normal.y, z = plane.normal.z, w = plane.distance;
            // note: a transpose is part of this transformation
            var a = ittrans.m00 * x + ittrans.m10 * y + ittrans.m20 * z + ittrans.m30 * w;
            var b = ittrans.m01 * x + ittrans.m11 * y + ittrans.m21 * z + ittrans.m31 * w;
            var c = ittrans.m02 * x + ittrans.m12 * y + ittrans.m22 * z + ittrans.m32 * w;
            var d = ittrans.m03 * x + ittrans.m13 * y + ittrans.m23 * z + ittrans.m33 * w;

            return new Plane(new Float3(a, b, c), d);
        }
        */

        // Creates a scaling matrix.
        public static Matrix4x4 Scale(Float3 vector)
        {
            Matrix4x4 m;
            m.M11 = vector.x; m.M12 = 0F; m.M13 = 0F; m.M14 = 0F;
            m.M21 = 0F; m.M22 = vector.y; m.M23 = 0F; m.M24 = 0F;
            m.M31 = 0F; m.M32 = 0F; m.M33 = vector.z; m.M34 = 0F;
            m.M41 = 0F; m.M42 = 0F; m.M43 = 0F; m.M44 = 1F;
            return m;
        }

        // Creates a translation matrix.
        public static Matrix4x4 Translate(Float3 vector)
        {
            Matrix4x4 m;
            m.M11 = 1F; m.M12 = 0F; m.M13 = 0F; m.M14 = vector.x;
            m.M21 = 0F; m.M22 = 1F; m.M23 = 0F; m.M24 = vector.y;
            m.M31 = 0F; m.M32 = 0F; m.M33 = 1F; m.M34 = vector.z;
            m.M41 = 0F; m.M42 = 0F; m.M43 = 0F; m.M44 = 1F;
            return m;
        }

        // Creates a rotation matrix. Note: Assumes unit quaternion
        public static Matrix4x4 Rotate(Quaternion q)
        {
            // Precalculate coordinate products
            float x = q.x * 2.0F;
            float y = q.y * 2.0F;
            float z = q.z * 2.0F;
            float xx = q.x * x;
            float yy = q.y * y;
            float zz = q.z * z;
            float xy = q.x * y;
            float xz = q.x * z;
            float yz = q.y * z;
            float wx = q.w * x;
            float wy = q.w * y;
            float wz = q.w * z;

            // Calculate 3x3 matrix from orthonormal basis
            Matrix4x4 m;
            m.M11 = 1.0f - (yy + zz); m.M21 = xy + wz; m.M31 = xz - wy; m.M41 = 0.0F;
            m.M12 = xy - wz; m.M22 = 1.0f - (xx + zz); m.M32 = yz + wx; m.M42 = 0.0F;
            m.M13 = xz + wy; m.M23 = yz - wx; m.M33 = 1.0f - (xx + yy); m.M43 = 0.0F;
            m.M14 = 0.0F; m.M24 = 0.0F; m.M34 = 0.0F; m.M44 = 1.0F;
            return m;
        }

        static readonly Matrix4x4 zeroMatrix = new Matrix4x4(new Float4(0, 0, 0, 0),
            new Float4(0, 0, 0, 0),
            new Float4(0, 0, 0, 0),
            new Float4(0, 0, 0, 0));

        public static Matrix4x4 zero { get { return zeroMatrix; } }

        static readonly Matrix4x4 identityMatrix = new Matrix4x4(
            new Float4(1, 0, 0, 0),
            new Float4(0, 1, 0, 0),
            new Float4(0, 0, 1, 0),
            new Float4(0, 0, 0, 1));

        public static Matrix4x4 identity { get { return identityMatrix; } }

    }
}
