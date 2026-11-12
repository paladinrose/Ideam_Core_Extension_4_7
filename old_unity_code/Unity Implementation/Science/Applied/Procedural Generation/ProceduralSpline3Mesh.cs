using System;
using System.Collections.Generic;
using System.Linq;

using Unity.Burst;
using Unity.Collections;
using Unity.Jobs;

using UnityEngine;
using UnityEngine.Rendering;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

using Matrix4x4 = UnityEngine.Matrix4x4;
using Quaternion = UnityEngine.Quaternion;
namespace Ideam.Unity.Science.Applied.ProceduralGeneration
{
    
    public class ProceduralSpline3Mesh : ProceduralMesh
    {
        #region Spline3 Management

        [SerializeField]
        protected Spline3 spline = null;
        public Spline3 Spline 
        { 
            get { return spline; } 
            set 
            {
                if (spline != null)
                {
                    if (value != null && spline.Length != value.Length || spline.Wrap != value.Wrap)
                    {
                        DestroyMesh();
                        CreateProceduralSpline3Mesh(value, radiusSpline, sides, evt => { }, useSmoothPoints);
                    }
                    else
                    {
                        UpdateMesh();
                    }
                }
                else if(value != null)
                {
                    CreateProceduralSpline3Mesh(value, radiusSpline, sides, evt => { }, useSmoothPoints);
                }
            }
        }

        public Vector3 SampleSpline(float percent)
        {
            return spline.GetPoint(percent).ToVector3();
        }

        public Vector3 GetSplinePoint(int id)
        {
            if (id < 0 || id >= spline.Length) { return Vector3.zero; }

            return spline[id].ToVector3();
        }

        public void SetSplinePoint(int id, Vector3 newPoint)
        {
            if (id < 0 || id >= spline.Length) { return; }
            Float3 np = newPoint.ToFloat3();
            if (spline[id] != np)
            {
                spline[id] = np;
                UpdateMesh();
            }
        }


        [SerializeField]
        protected Spline1 radiusSpline;
        public Spline1 RadiusSpline 
        {
            get { return radiusSpline; } 
            set 
            {
                if (radiusSpline != null)
                {
                    if (value != null && radiusSpline.Length != value.Length || radiusSpline.Wrap != value.Wrap)
                    {
                        DestroyMesh();
                        CreateProceduralSpline3Mesh(spline, value, sides, evt => { }, useSmoothPoints);
                    }
                    else
                    {
                        UpdateMesh();
                    }
                }
                else if (value != null)
                {
                    CreateProceduralSpline3Mesh(spline, value, sides, evt => { }, useSmoothPoints);
                }

            }
        }

        public float GetRadius(float percent)
        {
            float rad = radiusSpline.GetPoint(percent);

            return rad;
        }

        public void SetRadiusPoint(int id, float radius)
        {
            if (id < 0 || id >= radiusSpline.Length) { return; }
            radiusSpline[id] = radius;
        }

        #endregion


        [SerializeField]
        protected int sides = 1;
        public int Sides 
        {
            get { return sides; }
            set
            {
                if(value != sides)
                {
                    DestroyMesh();
                    CreateProceduralSpline3Mesh(spline, radiusSpline, value, evt => { }, useSmoothPoints);
                }
            }
        }

        [SerializeField]
        protected bool useSmoothPoints = true;
        public virtual bool UseSmoothPoints
        {
            get { return useSmoothPoints; }
            set
            {
                if(value != useSmoothPoints)
                {
                    DestroyMesh();
                    CreateProceduralSpline3Mesh(spline, radiusSpline, sides,evt => { }, value);
                }
            }
        }

        public int vertParameterID = -1, uvParameterID = -1, normalParameterID = -1, triangleParameterID = -1;

        public bool drawCapFaces;
        public Vector2 uvScale = Vector2.one;

        
        
        public virtual void CreateProceduralSpline3Mesh(Spline3 spline, Spline1 radiusSpline, int sides, Action<Mesh> returnAction, bool useSmoothPoints = true)
        {
            if(spline == null || radiusSpline == null || sides <= 0) { returnAction?.Invoke(null); return; }

            this.spline = spline;
            this.radiusSpline = radiusSpline;
            this.sides = sides;
            this.useSmoothPoints = useSmoothPoints;

            //First pass, let's just figure out how to do it as  regular function.
            Vector3[] points;
            float[] radii;
            if (useSmoothPoints) { points = spline.SmoothedPoints.ToVector3(); radii = radiusSpline.SmoothedPoints.Tofloat(); }
            else { points = spline.points.GetPoints().ToVector3(); radii = radiusSpline.points.GetPoints().Tofloat(); }

            Debug.Log("Point Count:" + points.Length + "  |   Radii Count: " + radii.Length);

            NativeArray<Vector3> splinePoints = new NativeArray<Vector3>(points, Allocator.TempJob);
            NativeArray<float> radiiPoints = new NativeArray<float>(radii, Allocator.TempJob);

            if (sides < 1) sides = 1;
            if(drawCapFaces && spline.Wrap) drawCapFaces = false;
            
            int numberOfColumns = sides + 1;
            int numberOfRows = points.Length + 1;

            //total number of vertices that make up the cylinder
            int vertCount = numberOfColumns * numberOfRows;

            //no of normals for each vertex
            int normalCount = vertCount;

            //uvs are always equal to no of vertices in a mesh
            int uvCount = vertCount;

            //side faces (tris) without the top and bottom caps
            int numberOfSideFaces = sides * points.Length * 2;

            //cap faces (2 caps bottom and top)
            int numberOfCapFaces = 0;
            if (drawCapFaces) numberOfCapFaces = sides - 2;

            //initialize all the arrays
            if (vertParameterID < 0)
            {
                bool makeVertParameter = true;

                for (int i = 0; i < parameters.Length; i++)
                {
                    if (parameters[i].parameterType == VertexAttribute.Position && parameters[i].streamID == 0)
                    {
                        vertParameterID = i;
                        makeVertParameter = false;
                        break;
                    }
                }

                if (makeVertParameter) 
                {
                    ProceduralMeshParameter vertParameter = new ProceduralMeshParameter();
                    vertParameterID = parameters.Length;
                    vertParameter.streamID = 0;
                    vertParameter.parameterType = VertexAttribute.Position;
                    System.Array.Resize(ref parameters, vertParameterID + 1);
                    parameters[vertParameterID] = vertParameter;
                }
            }

            Vector3[] vertices = new Vector3[vertCount];
            NativeArray<Vector3> verticesArray = new NativeArray<Vector3>(vertCount, Allocator.TempJob);

           
            if (normalParameterID < 0)
            {
                bool makeNormalParameter = true;

                for (int i = 0; i < parameters.Length; i++)
                {
                    if (parameters[i].parameterType == VertexAttribute.Normal && parameters[i].streamID == 0)
                    {
                        normalParameterID = i;
                        makeNormalParameter = false;
                        break;
                    }
                }

                if (makeNormalParameter)
                {
                    ProceduralMeshParameter normalParameter = new ProceduralMeshParameter();
                    normalParameterID = parameters.Length;
                    normalParameter.streamID = 0;
                    normalParameter.parameterType = VertexAttribute.Normal;
                    System.Array.Resize(ref parameters, normalParameterID + 1);
                    parameters[normalParameterID] = normalParameter;
                }
            }

            Vector3[] normals = new Vector3[normalCount];
            NativeArray<Vector3> normalsArray = new NativeArray<Vector3>(vertCount, Allocator.TempJob);
            
            if (uvParameterID < 0)
            {
                bool makeUVPartameter = true;

                for (int i = 0; i < parameters.Length; i++)
                {
                    if (parameters[i].IsUV && parameters[i].streamID == 0)
                    {
                        uvParameterID = i;
                        makeUVPartameter = false;
                        break;
                    }
                }

                if (makeUVPartameter)
                {
                    ProceduralMeshParameter uvParameter = new ProceduralMeshParameter();
                    uvParameterID = parameters.Length;
                    uvParameter.streamID = 0;
                    uvParameter.parameterType = VertexAttribute.TexCoord0;
                    System.Array.Resize(ref parameters, uvParameterID + 1);
                    parameters[uvParameterID] = uvParameter;
                }
            }

            Vector2[] uvs = new Vector2[uvCount];
            NativeArray<Vector2> uvsArray = new NativeArray<Vector2>(vertCount, Allocator.TempJob);

            
            if (triangleParameterID < 0)
            {
                bool makeTrianglePartameter = true;

                for (int i = 0; i < parameters.Length; i++)
                {
                    if (parameters[i].isIndexParameter )
                    {
                        triangleParameterID = i;
                        makeTrianglePartameter = false;
                        break;
                    }
                }

                if (makeTrianglePartameter)
                {
                    ProceduralMeshParameter triangleParameter = new ProceduralMeshParameter();
                    triangleParameterID = parameters.Length;
                    triangleParameter.isIndexParameter = true;

                    System.Array.Resize(ref parameters, triangleParameterID + 1);
                    parameters[triangleParameterID] = triangleParameter;
                }
            }

            int[] tris = new int[(numberOfSideFaces + numberOfCapFaces * 2) * 3];
            NativeArray<int> trianglesArray = new NativeArray<int>(tris.Length, Allocator.TempJob);

            
            //angle step for each column for side tris
            float angleStep = Mathf.PI * 2 / sides;

            Vector3[] unitPolygon = Spline2.NewPolygon(numberOfColumns).SmoothedPoints.ToVector3(XYToXYZ.XZ);
            NativeArray<Vector3> unitP = new NativeArray<Vector3>(unitPolygon, Allocator.TempJob);

            var createMeshJob = new CreateSpline3Mesh() 
            {
                normalsArray = normalsArray, 
                numberOfColumns = numberOfColumns,
                numberOfRows = numberOfRows,
                radii = radiiPoints,
                splinePoints = splinePoints,
                trianglesArray = trianglesArray,
                unitPolygon = unitP,
                uvsArray = uvsArray,
                uvScale = uvScale,
                verticesArray = verticesArray,
                wrap = spline.Wrap
            };

            var handle = createMeshJob.Schedule(numberOfRows, numberOfRows);

            handle.Complete();

            tris = trianglesArray.ToArray();

            if (drawCapFaces)
            {
                

                int firstIndex = 0;
                int midIndex = 0;
                int lastIndex = 0;
                int topCapOffset = vertCount - numberOfColumns;

                for (int i = 0; i < numberOfCapFaces; i++)
                {
                    //we get the bottom index to populate faces for bottom cap
                    int bottomIndex = i * 3;

                    //top cap tris will be stored in the empty end location of faces array
                    int topIndex = (numberOfCapFaces + numberOfSideFaces) * 3 + i * 3;

                    //get the three index for each vertex to make a cap tri
                    if (i == 0)
                    {
                        firstIndex = 1;
                        midIndex = 0;
                        lastIndex = numberOfColumns - 2;
                    }
                    else
                    {
                        midIndex = lastIndex;
                        lastIndex = lastIndex - 1;
                    }

                    //populate triangle vertices for bottom cap
                    tris[bottomIndex + 0] = lastIndex;
                    tris[bottomIndex + 1] = midIndex;
                    tris[bottomIndex + 2] = firstIndex;

                    //populate triangle vertices for top cap
                    tris[topIndex + 0] = topCapOffset + firstIndex;
                    tris[topIndex + 1] = topCapOffset + midIndex;
                    tris[topIndex + 2] = topCapOffset + lastIndex;
                }
            }

            mesh = new Mesh();
            mesh.vertices = verticesArray.ToArray();
            mesh.uv = uvsArray.ToArray();
            mesh.normals = normalsArray.ToArray();
            //mesh.triangles = trianglesArray.ToArray();
            mesh.triangles = tris;
            mesh.Optimize();

            verticesArray.Dispose();
            normalsArray.Dispose();
            uvsArray.Dispose();
            trianglesArray.Dispose();

            PrepareProceduralMesh(returnAction);
            //CreateProceduralMesh(mesh, returnAction);
        }

        public override void UpdateMesh()
        {
            if(spline == null || radiusSpline == null || mesh == null) { return; }

            if(meshComputeShader != null)
            {
                UpdateMeshComputeShader();
            }
            else
            {
                UpdateMeshJob();
            }
            base.UpdateMesh();
        }

        protected override void UpdateMeshJob()
        {
            Vector3[] points;
            float[] radii;
            if (useSmoothPoints) { points = spline.SmoothedPoints.ToVector3(); radii = radiusSpline.SmoothedPoints.Tofloat(); }
            else { points = spline.points.GetPoints().ToVector3(); radii = radiusSpline.points.GetPoints().Tofloat(); }

            NativeArray<Vector3> splinePoints = new NativeArray<Vector3>(points, Allocator.TempJob);
            NativeArray<float> radiiPoints = new NativeArray<float>(radii, Allocator.TempJob);

            Vector3[] unitPolygon = Spline2.NewPolygon(sides+1).SmoothedPoints.ToVector3(XYToXYZ.XZ);
            NativeArray<Vector3> unitP = new NativeArray<Vector3>(unitPolygon, Allocator.TempJob);

            SplineUpdateJob updateJob = new SplineUpdateJob()
            {
                splinePoints = splinePoints,
                radii = radiiPoints,
                sides = sides+1,
                unitPolygon = unitP,
                vertices = parameters[vertParameterID].vector3Array, 
                wrap = spline.Wrap
            };
            var handle = updateJob.Schedule(splinePoints.Length, splinePoints.Length);
            handle.Complete();
            base.UpdateMeshJob();
        }

        #region Jobs

        [BurstCompile]
        struct CreateSpline3Mesh : IJobParallelFor
        {
            [ReadOnly] public Vector2 uvScale;
            [ReadOnly] public int numberOfColumns, 
                                  numberOfRows;
            [ReadOnly] public bool wrap;

            [ReadOnly] [NativeDisableParallelForRestriction] public NativeArray<Vector3> splinePoints;
            [ReadOnly] public NativeArray<float> radii;
            [ReadOnly] public NativeArray<Vector3> unitPolygon;
            
            [WriteOnly] public NativeArray<Vector3> verticesArray;
            [WriteOnly] public NativeArray<Vector2> uvsArray;
            [WriteOnly] public NativeArray<Vector3> normalsArray;
            [WriteOnly] public NativeArray<int> trianglesArray;

            public void Execute(int i)
            {
                float rad = radii[i];
                if (rad < 0.00001f) rad = 0.00001f;

                Vector3 currentPoint = splinePoints[i];

                Vector3 nextPoint = Vector3.zero;
                if (i < numberOfRows - 1) { nextPoint = splinePoints[i + 1]; }
                else if (wrap) { nextPoint = splinePoints[0]; }
                else
                {
                    Vector3 lastDirection = currentPoint - splinePoints[i - 1];
                    nextPoint = currentPoint + lastDirection;
                }
                Quaternion direction = Quaternion.FromToRotation(Vector3.up, nextPoint - currentPoint);

                Matrix4x4 matrix = Matrix4x4.Scale(Vector3.one * rad);
                matrix *= Matrix4x4.Rotate(direction);
                matrix *= Matrix4x4.Translate(currentPoint);

                for (int j = 0; j < numberOfColumns; j++)
                {
                    int currentID = i * numberOfColumns + j;

                    verticesArray[currentID] = matrix.MultiplyPoint3x4(unitPolygon[j]);
                    uvsArray[currentID] = new Vector2(j * uvScale.x, i * uvScale.y);
                    normalsArray[currentID] = new Vector3(0, 0, -1.0f);

                    //If this isn't the last row.
                    if (i < numberOfRows - 1)
                    {
                        int triID = currentID * 3;

                        trianglesArray[triID] = currentID;

                        int offset = 0;

                        if(j == numberOfColumns-1)
                        {
                            offset = -numberOfColumns;
                        }

                        if(j%2 != 0)
                        {
                            trianglesArray[triID + 1] = currentID + numberOfColumns;
                            trianglesArray[triID + 2] = currentID + numberOfColumns - 1;
                        }
                        else
                        {
                            trianglesArray[triID + 1] = currentID + 1 + offset;
                            trianglesArray[triID + 2] = currentID + numberOfColumns;  
                        }
                    }
                }
            }
        }

        [BurstCompile]
        struct SplineUpdateJob : IJobParallelFor
        {
            [ReadOnly] [NativeDisableParallelForRestriction] public NativeArray<Vector3> splinePoints;
            [ReadOnly] public NativeArray<float> radii;
            [ReadOnly] public int sides;
            [ReadOnly] public bool wrap;
            [ReadOnly] public NativeArray<Vector3> unitPolygon;
            
            [WriteOnly] public NativeArray<Vector3> vertices;

            public void Execute(int index)
            {
                Vector3 point = splinePoints[index];
                Vector3 nextPoint = Vector3.zero;
                if(index < splinePoints.Length - 1) { nextPoint = splinePoints[index + 1]; }
                else if(wrap)
                {
                    nextPoint = splinePoints[0];
                }
                else
                {
                    nextPoint = point - splinePoints[index - 1];
                    nextPoint += point;
                }

                float radius = radii[index];

                int startVertID = index * sides;
                
                Quaternion direction = Quaternion.FromToRotation(Vector3.up, nextPoint-point);

                Matrix4x4 matrix = Matrix4x4.Scale(Vector3.one*radius);
                matrix *= Matrix4x4.Rotate(direction);
                matrix *= Matrix4x4.Translate(point);

                for(int i =0; i < sides; i++)
                {
                    vertices[i + startVertID] = matrix.MultiplyPoint3x4(unitPolygon[i]);
                }

            }
        }

        
        #endregion

        
    }

    public class CyliderGenerate
    {
        public Vector3 normalDirection = Vector3.up;
        public float radius = 3f;
        public int sides = 3;
        public int height = 2;
        public bool drawCapFaces;
        private const int minimumSides = 3;
        private const int minimumRadius = 1;

        void ComputeCylinder(out Vector3[] vertices, out Vector2[] uvs, out Vector3[] normals, out int[] faces)
        {

            if (sides < minimumSides) sides = minimumSides;
            if (radius < minimumRadius) radius = minimumRadius;

            //define total columns and rows
            int numberOfColumns = sides + 1;
            int numberOfRows = height + 1;

            //total number of vertices that make up the cylinder
            int vertCount = numberOfColumns * numberOfRows;

            //no of normals for each vertex
            int normalCount = vertCount;

            //uvs are always equal to no of vertices in a mesh
            int uvCount = vertCount;

            //side faces (tris) without the top and bottom caps
            int numberOfSideFaces = sides * height * 2;

            //cap faces (2 caps bottom and top)
            int numberOfCapFaces = 0;
            if(drawCapFaces) numberOfCapFaces = sides - 2;

            //initialize all the arrays
            vertices = new Vector3[vertCount];
            normals = new Vector3[normalCount];
            uvs = new Vector2[uvCount];
            faces = new int[(numberOfSideFaces + numberOfCapFaces * 2) * 3];

            //angle step for each column for side tris
            float angleStep = Mathf.PI * 2 / sides;

            /*
               first for loop computes all the side faces of the cylinder
               second loop computes tris for top and bottom caps
             */
            for (int i = 0; i < numberOfRows; i++)
            {
                for (int j = 0; j < numberOfColumns; j++)
                {
                    float angle = j * angleStep;

                    //folding from the first and last vertex
                    if (j == numberOfColumns - 1) angle = 0;

                    //compute vertices, uvs and normals for each row and column offsets
                    vertices[i * numberOfColumns + j] = new Vector3(radius * Mathf.Cos(angle), i * height, radius * Mathf.Sin(angle)); //build a cylinder with an upwards orientation
                    uvs[i * numberOfColumns + j] = new Vector2(j * 1 / radius, i * 1 / normalDirection.y);
                    normals[i * numberOfColumns + j] = new Vector3(0, 0, -1.0f);

                    /*
                       To create faces, we ignore the first row and the last column
                       for every other vertex we create two triangle faces at the same time in one loop
                     */
                    if (i != 0 && j < numberOfColumns - 1)
                    {
                        //offset the initial space for storing tris for bottom cap
                        int index = numberOfCapFaces * 3 + (i - 1) * sides * 6 + j * 6;

                        //create the first face
                        faces[index + 0] = i * numberOfColumns + j;
                        faces[index + 1] = i * numberOfColumns + j + 1;
                        faces[index + 2] = (i - 1) * numberOfColumns + j;

                        //create the second face
                        faces[index + 3] = (i - 1) * numberOfColumns + j;
                        faces[index + 4] = i * numberOfColumns + j + 1;
                        faces[index + 5] = (i - 1) * numberOfColumns + j + 1;
                    }
                }
            }

            /*drawing top and bottom caps
            we need the firstIndex, midIndex and lastIndex as vertices for cap tris and store it in the faces array*/
            if (drawCapFaces)
            {
                int firstIndex = 0;
                int midIndex = 0;
                int lastIndex = 0;
                int topCapOffset = vertCount - numberOfColumns;

                for (int i = 0; i < numberOfCapFaces; i++)
                {
                    //we get the bottom index to populate faces for bottom cap
                    int bottomIndex = i * 3;

                    //top cap tris will be stored in the empty end location of faces array
                    int topIndex = (numberOfCapFaces + numberOfSideFaces) * 3 + i * 3;

                    //get the three index for each vertex to make a cap tri
                    if (i == 0)
                    {
                        firstIndex = 1;
                        midIndex = 0;
                        lastIndex = numberOfColumns - 2;
                    }
                    else
                    {
                        midIndex = lastIndex;
                        lastIndex = lastIndex - 1;
                    }

                    //populate triangle vertices for bottom cap
                    faces[bottomIndex + 0] = lastIndex;
                    faces[bottomIndex + 1] = midIndex;
                    faces[bottomIndex + 2] = firstIndex;

                    //populate triangle vertices for top cap
                    faces[topIndex + 0] = topCapOffset + firstIndex;
                    faces[topIndex + 1] = topCapOffset + midIndex;
                    faces[topIndex + 2] = topCapOffset + lastIndex;
                }
            }
        }
    }
}