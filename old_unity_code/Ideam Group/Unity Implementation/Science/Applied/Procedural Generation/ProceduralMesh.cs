using System;
using System.Collections.Generic;
using System.Linq;

using Unity.Burst;
using Unity.Jobs;

using UnityEngine;
using UnityEngine.Rendering;

namespace Ideam.Unity.Science.Applied.ProceduralGeneration
{

    public class ProceduralMesh : ScriptableObject
    {
        // Step 1: Mesh Generation. This is when the number of verts and tris is going to change.
        // -It needs a function that receives the new mesh data. Should be virtual so we can create specific types of 
        //  mesh generators as befits the situation.
        // -It needs a job for building the mesh, tri indexes, and uvs.
        // -The memory allocations for this are dynamic and not kept around longer than the actual job.

        public virtual event Action<Mesh> OnMeshCreated, OnMeshUpdated, OnDestroyMesh;
        
        public int threadCount;

        protected int triangleCount;
        
        List<int> kernels;

        #region Procedural Mesh Parameters

        [SerializeField]
        protected ProceduralMeshParameter[] parameters = new ProceduralMeshParameter[0];
        public virtual ProceduralMeshParameter[] Parameters 
        {
            get { return parameters; } 
            set 
            {
                if(value != parameters)
                {
                    CleanUpGPUResources();
                    CleanUpJobResources();

                    parameters = value;
                    if(meshComputeShader == null)
                    {
                        CreateGPUResources();
                    }
                    else
                    {
                        CreateJobResources();
                    }
                }
            }
        }

        public virtual int AddParameter(ProceduralMeshParameter newParameter)
        {
            int id = GetParameterID(newParameter);
            if(id < 0)
            {
                int l = parameters.Length;
                System.Array.Resize(ref parameters, l + 1);
                parameters[l] = newParameter;
                return l;
            }
            return id;
        }

        public virtual int GetParameterID(ProceduralMeshParameter parameter)
        {
            for(int i =parameters.Length-1; i >=0; i--)
            {
                if (parameters[i] == parameter) return i;
            }
            return -1;
        }

        public virtual void RemoveParameter(ProceduralMeshParameter toRemove)
        {
            int idToRemove = GetParameterID(toRemove);
            RemoveParameter(idToRemove);
        }

        public virtual void RemoveParameter(int toRemove)
        {
            if(toRemove <0 || toRemove >= parameters.Length) { return; }
            
            List<ProceduralMeshParameter> newParameters = new List<ProceduralMeshParameter>(parameters);
            newParameters.RemoveAt(toRemove);

            parameters = newParameters.ToArray();

        }

        #endregion

        [SerializeField]
        protected ComputeShader meshComputeShader;
        public virtual ComputeShader MeshComputeShader 
        {
            get { return meshComputeShader; } 
            set 
            { 
                if (value != meshComputeShader) 
                {
                    if (value == null)
                    {
                        CleanUpGPUResources();
                        CreateJobResources();
                    }
                    else
                    {
                        CleanUpJobResources();
                        CreateGPUResources();
                    }
                    meshComputeShader = value;
                    
                }
            }
        }
        
        [SerializeField]
        protected Mesh mesh;
        public virtual Mesh Mesh 
        {
            get { return mesh; }
            set 
            { 
                if(mesh != value)
                {
                    DestroyMesh();
                }
                CreateProceduralMesh(value);
            }
        }
        
        public virtual void CreateProceduralMesh()
        {
            mesh = new Mesh();
            mesh.MarkDynamic();

            kernels = new List<int>();
            
            OnMeshCreated?.Invoke(mesh);
        }

        public virtual void CreateProceduralMesh(Mesh source, Action<Mesh> returnAction = null)
        {
            Vector3[] sourceVerts = source.vertices;
            triangleCount = source.triangles.Length / 3;
            mesh = new Mesh();

            mesh.MarkDynamic();

            mesh.vertices = sourceVerts;
            mesh.uv = source.uv;
            mesh.triangles = source.triangles;
            mesh.normals = source.normals;
            mesh.colors = source.colors;
        }

        public virtual void PrepareProceduralMesh(Action<Mesh> returnAction = null) 
        { 
            mesh.vertexBufferTarget |= GraphicsBuffer.Target.Raw;

            List<VertexAttributeDescriptor> vertexBufferParams = new List<VertexAttributeDescriptor>();
            
            bool setIndexBuffer = false;
            
            for(int i =0; i < parameters.Length; i++)
            {
                if (parameters[i].isIndexParameter) { setIndexBuffer = true; continue; }
                vertexBufferParams.Add(new VertexAttributeDescriptor(parameters[i].parameterType, stream: parameters[i].streamID));
            }
            
            mesh.SetVertexBufferParams(mesh.vertexCount, vertexBufferParams.ToArray());

            if(setIndexBuffer) mesh.SetIndexBufferParams(mesh.triangles.Length, IndexFormat.UInt16);

            if(meshComputeShader == null) { CreateJobResources(); } 
            else { CreateGPUResources(); }
            
            OnMeshCreated?.Invoke(mesh);
            
            returnAction.Invoke(mesh);
            
            UpdateMesh();
        }

        // Step 2: Mesh Updating. This is when the number of verts isn't changing, but their positions are.
            // -It needs a function to receive the new values.
            // -It needs a job to perform the updating. This job uses persistently allocated buffers, since only the info
            //  in the buffers will be changing.

        public virtual void UpdateMesh()
        {
            if(mesh == null) { return; }
            if (meshComputeShader != null) { UpdateMeshComputeShader(); }
            else { UpdateMeshJob(); }
            OnMeshUpdated?.Invoke(mesh);
        }

        protected virtual void OnDestroy()
        {
            DestroyMesh();
            CleanUpGPUResources();
            CleanUpJobResources();
        }

        public virtual void DestroyMesh()
        {
            if (mesh == null) return;

            OnDestroyMesh?.Invoke(mesh);

            IdeamUnity.Destroy(mesh);
            CleanUpGPUResources();
            CleanUpJobResources();
        }

        #region Jobs Methods

        protected virtual void CreateJobResources()
        {
            int l = mesh.vertices.Length;
            for (int i = 0; i < parameters.Length; i++)
            {
                ProceduralMeshParameter p = parameters[i];
                if (!p.isIndexParameter)
                {
                    p.UseJobs(l);
                }
                else
                {
                    p.UseJobs(mesh.triangles.Length);
                }
            }

        }

        protected virtual void CleanUpJobResources()
        {
            for (int i = 0; i < parameters.Length; i++)
            {
                parameters[i].DisposeJobsResources();
            }
        }

        protected virtual void UpdateMeshJob()
        {
            for (int i = 0; i < parameters.Length; i++)
            {
                ProceduralMeshParameter p = parameters[i];
                if (p.isIndexParameter)
                {
                    mesh.SetIndexBufferData(p.intArray, 0, 0, p.intArray.Length, MeshUpdateFlags.DontValidateIndices);
                }
                else
                {
                    switch (p.parameterType)
                    {
                        case VertexAttribute.Position:
                            mesh.SetVertexBufferData(p.vector3Array, 0, 0, p.vector3Array.Length, p.streamID);
                            break;
                        case VertexAttribute.Normal:
                            mesh.SetVertexBufferData(p.vector3Array, 0, 0, p.vector3Array.Length, p.streamID);
                            break;
                        case VertexAttribute.Tangent:
                            mesh.SetVertexBufferData(p.vector4Array, 0, 0, p.vector4Array.Length, p.streamID);
                            break;
                        case VertexAttribute.Color:
                            mesh.SetVertexBufferData(p.colorArray, 0, 0, p.colorArray.Length, p.streamID);
                            break;
                        case VertexAttribute.TexCoord0:
                            mesh.SetVertexBufferData(p.vector2Array, 0, 0, p.vector2Array.Length, p.streamID);
                            break;
                        case VertexAttribute.TexCoord1:
                            mesh.SetVertexBufferData(p.vector2Array, 0, 0, p.vector2Array.Length, p.streamID);
                            break;
                        case VertexAttribute.TexCoord2:
                            mesh.SetVertexBufferData(p.vector2Array, 0, 0, p.vector2Array.Length, p.streamID);
                            break;
                        case VertexAttribute.TexCoord3:
                            mesh.SetVertexBufferData(p.vector2Array, 0, 0, p.vector2Array.Length, p.streamID);
                            break;
                        case VertexAttribute.TexCoord4:
                            mesh.SetVertexBufferData(p.vector2Array, 0, 0, p.vector2Array.Length, p.streamID);
                            break;
                        case VertexAttribute.TexCoord5:
                            mesh.SetVertexBufferData(p.vector2Array, 0, 0, p.vector2Array.Length, p.streamID);
                            break;
                        case VertexAttribute.TexCoord6:
                            mesh.SetVertexBufferData(p.vector2Array, 0, 0, p.vector2Array.Length, p.streamID);
                            break;
                        case VertexAttribute.TexCoord7:
                            mesh.SetVertexBufferData(p.vector2Array, 0, 0, p.vector2Array.Length, p.streamID);
                            break;
                        case VertexAttribute.BlendWeight:
                            mesh.SetVertexBufferData(p.floatArray, 0, 0, p.floatArray.Length, p.streamID);
                            break;
                        case VertexAttribute.BlendIndices:
                            mesh.SetVertexBufferData(p.intArray, 0, 0, p.intArray.Length, p.streamID);
                            break;
                    }
                }
            }
        }


        #endregion

        #region Compute Shader Methods

        protected virtual void CreateGPUResources()
        {
            for (int i = 0; i < parameters.Length; i++)
            {
                parameters[i].UseComputeShader(i);
            }
        }

        protected virtual void CleanUpGPUResources()
        {
            for (int i = 0; i < parameters.Length; i++)
            {
                parameters[i].DisposeGPUResources();
            }
        }

        protected virtual void UpdateMeshComputeShader()
        {
            kernels.Clear();
            for(int i =0; i < parameters.Length; i++)
            {
                ProceduralMeshParameter p = parameters[i];
                p.graphicsBuffer ??= mesh.GetVertexBuffer(p.bufferID);
                meshComputeShader.SetBuffer(p.kernelID, p.parameterID, p.graphicsBuffer);

                if (!kernels.Contains(p.kernelID)) { kernels.Add(p.kernelID); }

            }

            for (int i = 0; i < kernels.Count; i++)
            {
                meshComputeShader.Dispatch(kernels[i], (triangleCount + threadCount - 1) / threadCount, 1, 1);
            }
        }

        #endregion

    }

}