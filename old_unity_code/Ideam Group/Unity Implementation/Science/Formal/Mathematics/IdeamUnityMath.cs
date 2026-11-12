using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;

using Matrix4x4 = UnityEngine.Matrix4x4;
using Quaternion = UnityEngine.Quaternion;

namespace Ideam.Unity.Science.Formal.Mathematics
{
    public static class IdeamUnityMath
    {
        public static Vector2[] ToVector2(this Float2[] floats)
        {
            Vector2[] vecs = new Vector2[floats.Length];
            for(int i =0; i < floats.Length; i++)
            {
                vecs[i] = floats[i].ToVector2();
            }
            return vecs;
        }

        public static Color ToColor(this Float4 float4)
        {
            return new Color(float4.x, float4.y, float4.z, float4.w);
        }

        public static Matrix4x4 RotateAround(Vector3 centerOfRotation, float angleOfRotation, Vector3 rotationAxis)
        {

            // This should calculate the resulting matrix, as described in the answer
            Matrix4x4 translationToCenterPoint = Matrix4x4.Translate(centerOfRotation);
            Matrix4x4 rotation = Matrix4x4.Rotate(Quaternion.AngleAxis(angleOfRotation, rotationAxis));
            Matrix4x4 translationBackToOrigin = Matrix4x4.Translate(-centerOfRotation);

            Matrix4x4 resultMatrix = translationToCenterPoint * rotation * translationBackToOrigin;

            return resultMatrix;

        }

        public static Vector3 RandomVector3(Vector3 a, Vector3 b)
        {
            return new Vector3
            (
                Random.Range(a.x, b.x),
                Random.Range(a.y, b.y),
                Random.Range(a.z, b.z)
            );
        }
    }
}