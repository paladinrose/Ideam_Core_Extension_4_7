using Unity.Collections;

using UnityEngine;
using UnityEngine.Rendering;

namespace Ideam.Unity.Science.Applied.ProceduralGeneration
{
    [System.Serializable]
    public class ProceduralMeshParameter
    {
        public bool isIndexParameter;
        public bool IsUV
        {
            get
            {
                if (isIndexParameter) { return false; }
                switch (parameterType)
                {
                    case VertexAttribute.Position: return false;

                    case VertexAttribute.Normal: return false;

                    case VertexAttribute.Tangent: return false;

                    case VertexAttribute.Color: return false;

                    case VertexAttribute.BlendWeight: return false;

                    case VertexAttribute.BlendIndices: return false;
                       
                    default: return true;
                }
            }
        }
        public VertexAttribute parameterType;

        public NativeArray<int> intArray;
        public NativeArray<float> floatArray;
        public NativeArray<Vector2> vector2Array;
        public NativeArray<Vector3> vector3Array;
        public NativeArray<Vector4> vector4Array;
        public NativeArray<Color> colorArray;

        public GraphicsBuffer graphicsBuffer;

        public string parameterName;
        public int parameterID, kernelID, streamID, bufferID;
        public void UseJobs(int length)
        {
            DisposeGPUResources();
            if (isIndexParameter)
            {
                intArray = new NativeArray<int>(length, Allocator.Persistent);
            }
            else
            {
                switch (parameterType)
                {
                    case VertexAttribute.Position:
                        vector3Array = new NativeArray<Vector3>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.Normal:
                        vector3Array = new NativeArray<Vector3>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.Tangent:
                        vector4Array = new NativeArray<Vector4>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.Color:
                        colorArray = new NativeArray<Color>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.TexCoord0:
                        vector2Array = new NativeArray<Vector2>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.TexCoord1:
                        vector2Array = new NativeArray<Vector2>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.TexCoord2:
                        vector2Array = new NativeArray<Vector2>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.TexCoord3:
                        vector2Array = new NativeArray<Vector2>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.TexCoord4:
                        vector2Array = new NativeArray<Vector2>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.TexCoord5:
                        vector2Array = new NativeArray<Vector2>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.TexCoord6:
                        vector2Array = new NativeArray<Vector2>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.TexCoord7:
                        vector2Array = new NativeArray<Vector2>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.BlendWeight:
                        floatArray = new NativeArray<float>(length, Allocator.Persistent);
                        break;
                    case VertexAttribute.BlendIndices:
                        intArray = new NativeArray<int>(length, Allocator.Persistent);
                        break;
                }
            }
        }

        public void UseComputeShader(int bufferID)
        {
            DisposeJobsResources();
            this.bufferID = bufferID;
            parameterID = Shader.PropertyToID(parameterName);
        }

        public void Dispose()
        {
            DisposeJobsResources();
            DisposeGPUResources();
        }
        
        public void DisposeGPUResources()
        {
            graphicsBuffer?.Dispose();
            graphicsBuffer = null;
        }

        public void DisposeJobsResources()
        {
            if (intArray.IsCreated) intArray.Dispose();
            if (floatArray.IsCreated) floatArray.Dispose();
            if (vector2Array.IsCreated) vector2Array.Dispose();
            if (vector3Array.IsCreated) vector3Array.Dispose();
            if (vector4Array.IsCreated) vector4Array.Dispose();
            if (colorArray.IsCreated) colorArray.Dispose();
        }
    }

}