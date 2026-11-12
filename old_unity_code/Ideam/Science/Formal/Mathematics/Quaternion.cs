using System;

namespace Ideam.Science.Formal.Mathematics
{
    [Serializable]
    public struct Quaternion : IEquatable<Quaternion>
    {
        public float x;
        public float y;
        public float z;
        public float w;

        // Access the x, y, z, w components using [0], [1], [2], [3] respectively.
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                    case 0: return x;
                    case 1: return y;
                    case 2: return z;
                    case 3: return w;
                    default:
                        throw new IndexOutOfRangeException("Invalid Quaternion index!");
                }
            }

            set
            {
                switch (index)
                {
                    case 0: x = value; break;
                    case 1: y = value; break;
                    case 2: z = value; break;
                    case 3: w = value; break;
                    default:
                        throw new IndexOutOfRangeException("Invalid Quaternion index!");
                }
            }
        }

        // Constructs new Quaternion with given x,y,z,w components.
        public Quaternion(float x, float y, float z, float w) { this.x = x; this.y = y; this.z = z; this.w = w; }

        // Set x, y, z and w components of an existing Quaternion.
        public void Set(float newX, float newY, float newZ, float newW)
        {
            x = newX;
            y = newY;
            z = newZ;
            w = newW;
        }

        static readonly Quaternion identityQuaternion = new Quaternion(0F, 0F, 0F, 1F);

        // The identity rotation (RO). This quaternion corresponds to "no rotation": the object
        public static Quaternion identity
        {
            get
            {
                return identityQuaternion;
            }
        }

        // Combines rotations /lhs/ and /rhs/.
        public static Quaternion operator *(Quaternion lhs, Quaternion rhs)
        {
            return new Quaternion(
                lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
                lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z,
                lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x,
                lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z);
        }

        // Rotates the point /point/ with /rotation/.
        public static Float3 operator *(Quaternion rotation, Float3 point)
        {
            float x = rotation.x * 2F;
            float y = rotation.y * 2F;
            float z = rotation.z * 2F;
            float xx = rotation.x * x;
            float yy = rotation.y * y;
            float zz = rotation.z * z;
            float xy = rotation.x * y;
            float xz = rotation.x * z;
            float yz = rotation.y * z;
            float wx = rotation.w * x;
            float wy = rotation.w * y;
            float wz = rotation.w * z;

            Float3 res;
            res.x = (1F - (yy + zz)) * point.x + (xy - wz) * point.y + (xz + wy) * point.z;
            res.y = (xy + wz) * point.x + (1F - (xx + zz)) * point.y + (yz - wx) * point.z;
            res.z = (xz - wy) * point.x + (yz + wx) * point.y + (1F - (xx + yy)) * point.z;
            return res;
        }

        // *undocumented*
        public const float kEpsilon = 0.000001F;

        // Is the dot product of two quaternions within tolerance for them to be considered equal?
        private static bool IsEqualUsingDot(float dot)
        {
            // Returns false in the presence of NaN values.
            return dot > 1.0f - kEpsilon;
        }

        // Are two quaternions equal to each other?
        public static bool operator ==(Quaternion lhs, Quaternion rhs)
        {
            return IsEqualUsingDot(Dot(lhs, rhs));
        }

        // Are two quaternions different from each other?
        public static bool operator !=(Quaternion lhs, Quaternion rhs)
        {
            // Returns true in the presence of NaN values.
            return !(lhs == rhs);
        }

        // The dot product between two rotations.
        public static float Dot(Quaternion a, Quaternion b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }

        //FIX
        public static Quaternion FromToRotation(Float3 fromDirection, Float3 toDirection)
        {
            return Quaternion.identity; 
        }
        //FIX
        public static Quaternion Inverse(Quaternion rotation)
        {
            Matrix4x4 fromRot = Matrix4x4.Rotate(rotation);
            fromRot = fromRot.Inverse();

            return Quaternion.identity;
        }

        //FIX
        public static Quaternion Slerp(Quaternion a, Quaternion b, float t)
        {
            return Lerp(a, b, t);
        }

        //FIX
        public static Quaternion SlerpUnclamped(Quaternion a, Quaternion b, float t)
        {
            return LerpUnclamped(a, b, t);
        }

        
        public static Quaternion Lerp(Quaternion a, Quaternion b, float t) 
        {
            t = Float.Clamp01(t);
            Quaternion lerped = new Quaternion
            (
                Float.Lerp(a.x, b.x, t),
                Float.Lerp(a.y, b.y, t),
                Float.Lerp(a.z, b.z, t),
                Float.Lerp(a.w, b.w, t)
            );
            return lerped;
        }

        public static Quaternion LerpUnclamped(Quaternion a, Quaternion b, float t)
        {
            Quaternion lerped = new Quaternion
            (
                Float.Lerp(a.x, b.x, t),
                Float.Lerp(a.y, b.y, t),
                Float.Lerp(a.z, b.z, t),
                Float.Lerp(a.w, b.w, t)
            );
            return lerped;
        }

        //FIX
        private static Quaternion Internal_FromEulerRad(Float3 euler)
        {
            return Quaternion.identity;
        }

        //FIX
        private static Float3 Internal_ToEulerRad(Quaternion rotation)
        {
            return Float3.zero;
        }
        
        //FIX
        private static void Internal_ToAxisAngleRad(Quaternion q, out Float3 axis, out float angle)
        {
            axis = Float3.zero;
            angle = 0;
        }

        //FIX
        public static Quaternion AngleAxis(float angle, Float3 axis)
        {
            return Quaternion.identity;
        }

        public static Quaternion LookRotation(Float3 forward, Float3 upwards)
        {
            return Quaternion.identity;
        }

        public static Quaternion LookRotation(Float3 forward) { return LookRotation(forward, Float3.up); }

        public void SetLookRotation(Float3 view)
        {
            Float3 up = Float3.up;
            SetLookRotation(view, up);
        }

        // Creates a rotation with the specified /forward/ and /upwards/ directions.
        public void SetLookRotation(Float3 view, Float3 up)
        {
            this = LookRotation(view, up);
        }

        public static float Angle(Quaternion a, Quaternion b)
        {
            float dot = Dot(a, b);
            return IsEqualUsingDot(dot) ? 0.0f : Float.Acos(Float.Min(Float.Abs(dot), 1.0F)) * 2.0F * Float.RadToDeg;
        }

        // Makes euler angles positive 0/360 with 0.0001 hacked to support old behaviour of QuaternionToEuler
        private static Float3 Internal_MakePositive(Float3 euler)
        {
            float negativeFlip = -0.0001f * Float.RadToDeg;
            float positiveFlip = 360.0f + negativeFlip;

            if (euler.x < negativeFlip)
                euler.x += 360.0f;
            else if (euler.x > positiveFlip)
                euler.x -= 360.0f;

            if (euler.y < negativeFlip)
                euler.y += 360.0f;
            else if (euler.y > positiveFlip)
                euler.y -= 360.0f;

            if (euler.z < negativeFlip)
                euler.z += 360.0f;
            else if (euler.z > positiveFlip)
                euler.z -= 360.0f;

            return euler;
        }

        public Float3 eulerAngles
        {
            get { return Internal_MakePositive(Internal_ToEulerRad(this) * Float.RadToDeg); }
            set { this = Internal_FromEulerRad(value * Float.DegToRad); }
        }
        public static Quaternion Euler(float x, float y, float z) { return Internal_FromEulerRad(new Float3(x, y, z) * Float.DegToRad); }
        public static Quaternion Euler(Float3 euler) { return Internal_FromEulerRad(euler * Float.DegToRad); }
       

        public void ToAngleAxis(out float angle, out Float3 axis) { Internal_ToAxisAngleRad(this, out axis, out angle); angle *= Float.RadToDeg; }
        public void SetFromToRotation(Float3 fromDirection, Float3 toDirection) { this = FromToRotation(fromDirection, toDirection); }

        public static Quaternion RotateTowards(Quaternion from, Quaternion to, float maxDegreesDelta)
        {
            float angle = Quaternion.Angle(from, to);
            if (angle == 0.0f) return to;
            return SlerpUnclamped(from, to, Float.Min(1.0f, maxDegreesDelta / angle));
        }

        public static Quaternion Normalize(Quaternion q)
        {
            float mag = Float.Sqrt(Dot(q, q));

            if (mag < Float.Epsilon)
                return Quaternion.identity;

            return new Quaternion(q.x / mag, q.y / mag, q.z / mag, q.w / mag);
        }

        public void Normalize()
        {
            this = Normalize(this);
        }

        public Quaternion normalized
        {
            get { return Normalize(this); }
        }

        // used to allow Quaternions to be used as keys in hash tables
        public override int GetHashCode()
        {
            return x.GetHashCode() ^ (y.GetHashCode() << 2) ^ (z.GetHashCode() >> 2) ^ (w.GetHashCode() >> 1);
        }

        // also required for being able to use Quaternions as keys in hash tables
        public override bool Equals(object other)
        {
            if (!(other is Quaternion)) return false;

            return Equals((Quaternion)other);
        }

        public bool Equals(Quaternion other)
        {
            return x.Equals(other.x) && y.Equals(other.y) && z.Equals(other.z) && w.Equals(other.w);
        }

    }
}
