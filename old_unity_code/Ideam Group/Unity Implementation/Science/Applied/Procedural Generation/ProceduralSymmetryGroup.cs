using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

using Unity.Collections;
using Unity.Burst;
using Unity.Jobs;
using Unity.Mathematics;

using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling;

namespace Ideam.Unity.Science.Applied.ProceduralGeneration
{

    public class ProceduralSymmetryGroup : ScriptableObject
    {
        [SerializeField]
        protected ComplexSymmetryGroup symmetryGroup;

        public IEnumerator TileRegion<T>(Spline2[] region, Float2 startingPoint, SymmetryGroupType tilingType)
        where T: struct, ISymmetry
        {

            T symmetry = (T)Topology.GetSymmetryTemplate(tilingType);
            symmetry.Position = startingPoint;

            List<T> newSymmetries = new List<T>();
            List<T> resolvedSymmetries = new List<T>();

            List<Float2> vertsList = new List<Float2>();
            List<SymmetryEdge> edgesList = new List<SymmetryEdge>();

            Float2[] primitiveVerts = Topology.GetSymmetryVertices(tilingType);
            Float2[] vertices = new Float2[primitiveVerts.Length];

            Matrix3x3 g2l = Matrix3x3.GlobalToLocalMatrix(symmetry.Angle, symmetry.Scale, symmetry.Position);

            Matrix3x3[] tilingTransformations = Topology.GetSymmetryTransformations(tilingType);
            for (int i = vertices.Length - 1; i >= 0; i--)
            {
                vertices[i] = primitiveVerts[i] * g2l;
            }

            Float2 minSize = new Float2(startingPoint), maxSize = new Float2(startingPoint);
            int l = symmetry.Vertices.Length;
            int[] vertIDs = new int[l];
            int[] neighborIDs = new int[symmetry.Vertices.Length];
            int[] edgeIDs = new int[symmetry.Vertices.Length];

            for (int i = 0; i < vertices.Length; i++)
            {
                vertIDs[i] = vertsList.Count;
                vertsList.Add(vertices[i]);
                if (minSize.x < vertices[i].x) { minSize.x = vertices[i].x; }
                if (maxSize.x > vertices[i].x) { maxSize.x = vertices[i].x; }

                if (minSize.y < vertices[i].y) { minSize.y = vertices[i].y; }
                if (maxSize.y > vertices[i].y) { maxSize.y = vertices[i].y; }
                
            }

            Float2 symmetrySize = (minSize - maxSize) * 0.5f;
            symmetrySize = new Float2(Float.Abs(symmetrySize.x), Float.Abs(symmetrySize.y));

            symmetry.Vertices = vertIDs;

            //Was GenerateSymmetryEdges
            SymmetryEdge[] edges = new SymmetryEdge[vertIDs.Length];
            
            for (int i = 0; i < vertIDs.Length - 1; i++)
            {
                edges[i] = new SymmetryEdge(vertIDs[i], vertIDs[i + 1]);
                edges[i].nodeA = 0;
                edgeIDs[i] = edgesList.Count;
                edgesList.Add(edges[i]);
            }

            edges[vertIDs.Length - 1] = new SymmetryEdge(vertIDs[vertIDs.Length - 1], vertIDs[0]);
            edgeIDs[vertIDs.Length - 1] = edgesList.Count;
            edgesList.Add(edges[vertIDs.Length - 1]);

            symmetry.Edges = edgeIDs;

            for (int i = neighborIDs.Length - 1; i >= 0; i--)
            {
                neighborIDs[i] = -1;
            }
            symmetry.Neighbors = neighborIDs;

            newSymmetries.Add(symmetry);

            /*
            Plain English:

            1. Create the first sym. Add it to the newSymmetries list.
            2. While newSymmetries still has members in it, repeat the rest of these steps:
            3. Get the first sym in the newSymmetries list. We'll call it currentSymmetry. For each neighbor that is still set to -1, generate that neighbor.
            4. See if that generated neighbor is in the resolvedSymmetries list. If it is, set the currentSymmetry's neighbor ID to that, note that it's alreay in the resolved set, and move on.
            5. If the generated neighbor is NOT in resolvedSymmetries, check to see if it's already in the newSymmetries list. If it is, calculate the neighborID and set it for currentSymmetry. Move on to next neighbor.
            6. If it's NOT in newSymmetries, we check to see if it's Position is within the region. If not, we set the neighborID to -2 (our shorthand for Out Of Region) and move on.
            7. If the generated neighbor IS within the region, we'll be adding it to newSymmetries. Then we generate it's verts. If any of its verts are OUTSIDE of the region, we change our minds about adding the neighbor, set the neighborID to -2, and move on to the next neighbor.
            8. If all the verts are within the region, we're still adding it. We calculate its neighborID.
            9. Now we set all of its vertIDs to the right id numbers from our master verts list: first, we see if the vert is already in the list. If so, that's our id number. If not, we add its ID as vertsList.count, then add it to vertsList. Now we set the vertIDs back on the generated neighbor.
            10. Now we generate its edges. We check to see each generated edge against the edges we've already added. If we've already added this edge (both have both the same verts, even if their order is reversed), we update that edges info with its new secondary node, set its edgeID on the generated neighbor, and move on. Otherwise, we add the newly generated edge and its edgeID.
            11. We add the generated neighbor to openSymmetries. 
            12. Once all the currentSymmetry's neighbors have been dealt with, we add it to resolvedSymmetries.


            Now, to jobify it!

            First, we need two sets of neighbors: TransformNeighbors and EdgeNeighbors. 

            1. Something is a TransformNeighbor if one of the symmetryGroup's transformations can move us to another symmetry, and a transformation (can be the same one) moves us back to the original symmetry.
            2. Symmetries are EdgeNeighbors if they share an edge. Simple.
            
            while (newSymmetries.Count > 0)
            {
                ISymmetry currentSymmetry = newSymmetries[0];
                newSymmetries.RemoveAt(0);

                int currentID = resolvedSymmetries.Count;

                neighborIDs = currentSymmetry.Neighbors;

                for (int j = 0; j < neighborIDs.Length; j++)
                {
                    if (neighborIDs[j] == -1)
                    {
                        ISymmetry neighborSymmetry = GetNeighbor(currentSymmetry, currentID, j);

                        bool neighborInClosedSet = false;

                        for (int k = closedSymmetries.Count - 1; k >= 0; k--)
                        {
                            if (WithinTolerance(neighborSymmetry, closedSymmetries[k]))
                            {
                                neighborInClosedSet = true;
                                neighborIDs[j] = k;
                                break;
                            }
                        }

                        if (!neighborInClosedSet)
                        {
                            bool neighborInOpenSet = false;

                            for (int k = openSymmetries.Count - 1; k >= 0; k--)
                            {
                                if (WithinTolerance(neighborSymmetry, openSymmetries[k]))
                                {
                                    neighborInOpenSet = true;
                                    neighborIDs[j] = currentID + k;
                                    break;
                                }
                            }

                            if (neighborInOpenSet) { continue; }

                            if (region.ContainsPoint(neighborSymmetry.Position) >= 0)
                            {
                                bool addSym = true;
                                Float2[] neighborVertices = GenerateSymmetryVertices(neighborSymmetry);
                                if (allPointsCoverage)
                                {
                                    for (int k = 0; k < neighborVertices.Length; k++)
                                    {
                                        if (region.ContainsPoint(neighborVertices[k]) < 0)
                                        {
                                            neighborIDs[j] = -2;
                                            addSym = false;
                                            break;
                                        }
                                    }
                                }
                                if (!addSym) { continue; }
                                neighborIDs[j] = currentID + openSymmetries.Count;

                                int[] neighborVertIDs = neighborSymmetry.Vertices;

                                for (int k = 0; k < neighborVertices.Length; k++)
                                {
                                    bool hasPoint = false;

                                    for (int l = vertsList.Count - 1; l >= 0; l--)
                                    {
                                        if (neighborVertices[k].InTolerance(vertsList[l], PositionTolerance))
                                        {
                                            hasPoint = true;
                                            neighborVertIDs[k] = l;
                                            break;
                                        }
                                    }

                                    if (!hasPoint)
                                    {
                                        neighborVertIDs[k] = vertsList.Count;
                                        vertsList.Add(neighborVertices[k]);
                                    }
                                }

                                neighborSymmetry.Vertices = neighborVertIDs;

                                int[] edgeIDS = neighborSymmetry.Edges;
                                SymmetryEdge[] symEdges = GenerateSymmetryEdges(neighborSymmetry, neighborIDs[j]);
                                for (int e = 0; e < symEdges.Length; e++)
                                {
                                    bool foundEdge = false;
                                    for (int k = edgesList.Count - 1; k >= 0; k--)
                                    {
                                        if (symEdges[e] == edgesList[k])
                                        {
                                            edgeIDS[e] = k;
                                            foundEdge = true;
                                            SymmetryEdge nE = edgesList[k];
                                            nE.nodeB = neighborIDs[j];
                                            edgesList[k] = nE;
                                            break;
                                        }
                                    }

                                    if (!foundEdge)
                                    {
                                        edgeIDS[e] = edgesList.Count;
                                        edgesList.Add(symEdges[e]);
                                    }
                                }
                                neighborSymmetry.Edges = edgeIDS;

                                openSymmetries.Add(neighborSymmetry);
                            }
                            else
                            {
                                neighborIDs[j] = -2;
                            }
                        }
                    }
                }

                currentSymmetry.Neighbors = neighborIDs;
                resolvedSymmetries.Add(currentSymmetry);

            }
            */
            yield return null;

        }
        
        #region Jobs
        struct EvaluateNewSymmetries<T> : IJobParallelForBatch
            where T: unmanaged, ISymmetry
        {
            [ReadOnly] public NativeArray<T> newNeighbors;
            [WriteOnly] public NativeList<T> resolvedNeighbors;

            public void Execute(int startIndex, int count)
            {
                int end = startIndex + count;
                for(int i = startIndex; i < end; i++)
                {
                    T newSym = newNeighbors[i];
                }
            }
        }

        #endregion
    }
}
