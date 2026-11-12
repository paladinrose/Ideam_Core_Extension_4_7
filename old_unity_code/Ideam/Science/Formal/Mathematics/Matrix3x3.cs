using System;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Matrix3x3
    {
        public Float M11, M12, M13;
        public Float M21, M22, M23;
        public Float M31, M32, M33;
       
       
        #region Constructors 
        public Matrix3x3(float m)
        {
            M11 = M12 = M13 = M21 = M22 = M23 = M31 = M32 = M33 = m;
        }

        public Matrix3x3(float m11, float m12, float m13,
                          float m21, float m22, float m23,
                          float m31, float m32, float m33)
        {
            M11 = m11; M12 = m12; M13 = m13;
            M21 = m21; M22 = m22; M23 = m23;
            M31 = m31; M32 = m32; M33 = m33;
        }

        public Matrix3x3(Matrix3x3 m)
        {
            M11 = m.M11; M12 = m.M12; M13 = m.M13;
            M21 = m.M21; M22 = m.M22; M23 = m.M23;
            M31 = m.M31; M32 = m.M32; M33 = m.M33;
        }
        #endregion

        #region Modified Matrix Functions

        public float Determinant()
        {
            float a = M11 * ((M22 * M33) - (M23 * M32)),
                  b = M21 * ((M21 * M33) - (M23 * M31)),
                  c = M31 * ((M21 * M32) - (M22 * M31));
            return a - b + c;
        }
        public static Matrix3x3 Identity()
        {
            Matrix3x3 identity = new Matrix3x3();
            identity.M11 = 1; identity.M22 = 1; identity.M33 = 1;
            return identity;
        }

        public static Matrix3x3 Minors(Matrix3x3 m)
        {
            Matrix3x3 minor = new Matrix3x3();

            minor.M11 = (m.M22 * m.M33) - (m.M23 * m.M32);
            minor.M12 = (m.M21 * m.M33) - (m.M23 * m.M31);
            minor.M13 = (m.M21 * m.M32) - (m.M22 * m.M31);

            minor.M21 = (m.M12 * m.M33) - (m.M13 * m.M32);
            minor.M22 = (m.M11 * m.M33) - (m.M13 * m.M31);
            minor.M23 = (m.M11 * m.M32) - (m.M12 * m.M31);

            minor.M31 = (m.M12 * m.M23) - (m.M13 * m.M22);
            minor.M32 = (m.M11 * m.M23) - (m.M13 * m.M21);
            minor.M33 = (m.M11 * m.M22) - (m.M12 * m.M21);
            return minor;
        }

        public static Matrix3x3 Cofactors(Matrix3x3 m)
        {
            Matrix3x3 cofactors = Minors(m);
            cofactors.M12 *= -1f;
            cofactors.M21 *= -1f;
            cofactors.M23 *= -1f;
            cofactors.M32 *= -1f;
            return cofactors;
        }

        public static Matrix3x3 Adjoined(Matrix3x3 m, bool isCofactors = false)
        {
            Matrix3x3 adjoined = new Matrix3x3(m);
            if (!isCofactors)
            {
                adjoined = Cofactors(m);
            }
            float swap = adjoined.M12;
            adjoined.M12 = adjoined.M21;
            adjoined.M21 = swap;

            swap = adjoined.M13;
            adjoined.M13 = adjoined.M31;
            adjoined.M31 = swap;

            swap = adjoined.M23;
            adjoined.M23 = adjoined.M32;
            adjoined.M32 = swap;
            return adjoined;
        }

        public static Matrix3x3 Inverse(Matrix3x3 m)
        {
            Matrix3x3 cofactors = Cofactors(m);
            Matrix3x3 inverted = Adjoined(cofactors, true);

            float determinate = 1f / ((m.M11 * cofactors.M11) + (m.M12 * cofactors.M12) + (m.M13 * cofactors.M13));

            return inverted * determinate;
        }

        #endregion

        #region Affine Transformations

        public static Matrix3x3 GlobalToLocalMatrix(float angle, Float2 scale, Float2 position)
        {
            Matrix3x3 rotationMatrix = RotationMatrix(angle);

            Matrix3x3 scaleMatrix = ScaleMatrix(scale);

            Matrix3x3 positionMatrix = TranslationMatrix(position);

            return positionMatrix * rotationMatrix * scaleMatrix;
        }

        public static Matrix3x3 TranslationMatrix(Float2 position)
        {
            Matrix3x3 positionMatrix = Matrix3x3.Identity();
            positionMatrix.M13 = position.x;
            positionMatrix.M23 = position.y;

            return positionMatrix;
        }

        public static Matrix3x3 RotationMatrix(Float angle)
        {
            Matrix3x3 rotationMatrix = Matrix3x3.Identity();
            float ang = Float.DegToRad * angle;
            rotationMatrix.M11 = Float.Cos(ang);
            rotationMatrix.M12 = Float.Sin(ang);
            rotationMatrix.M21 = -Float.Sin(ang);
            rotationMatrix.M22 = Float.Cos(ang);

            return rotationMatrix;
        }

        public static Matrix3x3 RotationAroundPointMatrix(Float2 point, float angle)
        {
            angle = Float.DegToRad * angle;

            Matrix3x3 r = RotationMatrix(angle);


            Matrix3x3 inverseT = TranslationMatrix(-point);

            Matrix3x3 t = TranslationMatrix(point);

            return t * r * inverseT;
        }

        public static Matrix3x3 ScaleMatrix(Float2 scale)
        {
            Matrix3x3 scaleMatrix = Matrix3x3.Identity();
            scaleMatrix.M11 = scale.x;
            scaleMatrix.M22 = scale.y;

            return scaleMatrix;
        }

        public static Matrix3x3 ReflectionMatrix(Float2 reflectLineStart, Float2 reflectLineEnd)
        {

            float x11, x12, x21, x22;  // Elements of the reflection matrix

            if (reflectLineStart.x == reflectLineEnd.x)
            {
                // If the reflection line is vertical
                x11 = -1;
                x12 = 0;
                x21 = 0;
                x22 = 1;
            }
            else if (reflectLineStart.y == reflectLineEnd.y)
            {
                // If the reflection line is horizontal
                x11 = 1;
                x12 = 0;
                x21 = 0;
                x22 = -1;
            }
            else
            {
                float at = (float)Math.Atan((reflectLineEnd.y - reflectLineStart.y) / (reflectLineEnd.x - reflectLineStart.x));

                x11 = (float)Math.Cos(2f * at);
                x12 = (float)Math.Sin(2f * at);
                x21 = x12;
                x22 = -x11;
            }

            Matrix3x3 reflectionMatrix = Matrix3x3.Identity();

            reflectionMatrix.M11 = x11;
            reflectionMatrix.M12 = x12;
            reflectionMatrix.M21 = x21;
            reflectionMatrix.M22 = x22;

            return reflectionMatrix;
        }
        public static Matrix3x3 GlideReflectionMatrix(Float2 reflectLineStart, Float2 reflectLineEnd, Float2 glide)
        {

            float x11, x12, x21, x22;  // Elements of the reflection matrix

            if (reflectLineStart.x == reflectLineEnd.x)
            {
                // If the reflection line is vertical
                x11 = -1;
                x12 = 0;
                x21 = 0;
                x22 = 1;
            }
            else if (reflectLineStart.y == reflectLineEnd.y)
            {
                // If the reflection line is horizontal
                x11 = 1;
                x12 = 0;
                x21 = 0;
                x22 = -1;
            }
            else
            {
                float at = (float)Math.Atan((reflectLineEnd.y - reflectLineStart.y) / (reflectLineEnd.x - reflectLineStart.x));

                x11 = (float)Math.Cos(2f * at);
                x12 = (float)Math.Sin(2f * at);
                x21 = x12;
                x22 = -x11;
            }

            Matrix3x3 reflectionMatrix = Matrix3x3.Identity();

            reflectionMatrix.M11 = x11;
            reflectionMatrix.M12 = x12;
            reflectionMatrix.M21 = x21;
            reflectionMatrix.M22 = x22;

            return TranslationMatrix(glide) * reflectionMatrix ;
        }

        #endregion

        #region Operators
        public static Matrix3x3 operator *(Matrix3x3 a, Matrix3x3 b)
        {
            var result = new Matrix3x3();

            result.M11 = a.M11 * b.M11 + a.M12 * b.M21 + a.M13 * b.M31;
            result.M12 = a.M11 * b.M12 + a.M12 * b.M22 + a.M13 * b.M32;
            result.M13 = a.M11 * b.M13 + a.M12 * b.M23 + a.M13 * b.M33;

            result.M21 = a.M21 * b.M11 + a.M22 * b.M21 + a.M23 * b.M31;
            result.M22 = a.M21 * b.M12 + a.M22 * b.M22 + a.M23 * b.M32;
            result.M23 = a.M21 * b.M13 + a.M22 * b.M23 + a.M23 * b.M33;

            result.M31 = a.M31 * b.M11 + a.M32 * b.M21 + a.M33 * b.M31;
            result.M32 = a.M31 * b.M12 + a.M32 * b.M22 + a.M33 * b.M32;
            result.M33 = a.M31 * b.M13 + a.M32 * b.M23 + a.M33 * b.M33;

            return result;
        }

        public static Float2 operator *(Float2 f, Matrix3x3 m)
        {
            float x, y, b1 = f.x, b2 = f.y;
            x = m.M11 * b1 + m.M12 * b2 + m.M13;
            y = m.M21 * b1 + m.M22 * b2 + m.M23;
            return new Float2(x, y);
        }

        public static Matrix3x3 operator *(Matrix3x3 a, float b)
        {
            Matrix3x3 result = new Matrix3x3();

            result.M11 = a.M11 * b; result.M12 = a.M12 * b; result.M13 = a.M13 * b;
            result.M21 = a.M11 * b; result.M22 = a.M22 * b; result.M23 = a.M23 * b;
            result.M31 = a.M11 * b; result.M32 = a.M32 * b; result.M33 = a.M33 * b;

            return result;
        }

        public static Matrix3x3 operator +(Matrix3x3 a, Matrix3x3 b)
        {
            Matrix3x3 result = new Matrix3x3();

            result.M11 = a.M11 + b.M11; result.M12 = a.M12 + b.M12; result.M13 = a.M13 + b.M13;
            result.M21 = a.M21 + b.M21; result.M22 = a.M22 + b.M22; result.M23 = a.M23 + b.M23;
            result.M31 = a.M31 + b.M31; result.M32 = a.M32 + b.M32; result.M33 = a.M33 + b.M33;

            return result;
        }

        public static Matrix3x3 operator +(Matrix3x3 a, float b)
        {
            Matrix3x3 result = new Matrix3x3();

            result.M11 = a.M11 + b; result.M12 = a.M12 + b; result.M13 = a.M13 + b;
            result.M21 = a.M21 + b; result.M22 = a.M22 + b; result.M23 = a.M23 + b;
            result.M31 = a.M31 + b; result.M32 = a.M32 + b; result.M33 = a.M33 + b;

            return result;
        }

        public static bool operator ==(Matrix3x3 a, Matrix3x3 b)
        {
            if (!a.M11.NearlyEqual(b.M11) ||
                !a.M12.NearlyEqual(b.M12) ||
                !a.M13.NearlyEqual(b.M13) ||
                !a.M21.NearlyEqual(b.M21) ||
                !a.M22.NearlyEqual(b.M22) ||
                !a.M23.NearlyEqual(b.M23) ||
                !a.M31.NearlyEqual(b.M31) ||
                !a.M32.NearlyEqual(b.M32) ||
                !a.M33.NearlyEqual(b.M33) 
                ) { return false; }
            return true;
        }
        public static bool operator !=(Matrix3x3 a, Matrix3x3 b)
        {
            
            if (!a.M11.NearlyEqual(b.M11) ||
                !a.M12.NearlyEqual(b.M12) ||
                !a.M13.NearlyEqual(b.M13) ||
                !a.M21.NearlyEqual(b.M21) ||
                !a.M22.NearlyEqual(b.M22) ||
                !a.M23.NearlyEqual(b.M23) ||
                !a.M31.NearlyEqual(b.M31) ||
                !a.M32.NearlyEqual(b.M32) ||
                !a.M33.NearlyEqual(b.M33)
                ) { return true; }
            return false;
        }

        public override bool Equals(object obj)
        {
            if(obj is Matrix3x3 m) { return m == this; }
            return false;
        }

        public override int GetHashCode()
        {
            var hashCode = M11.GetHashCode();
            hashCode = (hashCode * 397) ^ M12.GetHashCode();
            hashCode = (hashCode * 397) ^ M13.GetHashCode();
            hashCode = (hashCode * 397) ^ M21.GetHashCode();
            hashCode = (hashCode * 397) ^ M22.GetHashCode();
            hashCode = (hashCode * 397) ^ M23.GetHashCode();
            hashCode = (hashCode * 397) ^ M31.GetHashCode();
            hashCode = (hashCode * 397) ^ M32.GetHashCode();
            hashCode = (hashCode * 397) ^ M33.GetHashCode();
            return hashCode;
        }
        #endregion

    }
}
