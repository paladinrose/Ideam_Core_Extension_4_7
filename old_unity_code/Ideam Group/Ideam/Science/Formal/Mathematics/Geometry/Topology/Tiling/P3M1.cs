using System;
using System.Collections;
using System.Collections.Generic;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

using UnityEngine;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    [System.Serializable]
    public class P3M1 : HexagonalSymmetryGroup<P3M1,Symmetry3>
    {
        float positionTolerance = 0.01f, rotationTolerance = 1f, scaleTolerance = 0.01f;

        public override float PositionTolerance
        {
            get { return positionTolerance; }

            set
            {
                positionTolerance = value;
                if (positionTolerance < 0.01f) { positionTolerance = 0.01f; }
            }
        }

        public override float RotationTolerance
        {
            get { return rotationTolerance; }

            set
            {
                rotationTolerance = value;
                if (rotationTolerance < 1f) { rotationTolerance = 1f; }
            }
        }

        public override float ScaleTolerance
        {
            get { return scaleTolerance; }

            set
            {
                scaleTolerance = value;
                if (scaleTolerance < 0.01f) { scaleTolerance = 0.01f; }
            }
        }

        public override Float2[] PrimitiveVertices
        {
            get
            {
                return Topology.EquilateralTriangle();
            }
        }

        public P3M1() : base() { }
        public P3M1(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }

        public override Symmetry3 GetNeighbor(Symmetry3 baseCell, int cellID, int neighborID)
        { 
            Symmetry3 newNeighbor = new Symmetry3() 
            {
                position = baseCell.position,
                scale = baseCell.scale,
                angle = baseCell.angle,
                Generation = baseCell.Generation + 1
            };

            int[] neighbors = new int[] { -1, -1, -1 };

            Float2 scale = baseCell.Scale;

            switch (neighborID)
            {
                case 0:
                    Translate(ref newNeighbor, Float2.down * 0.5775f, true);
                    scale.y *= -1;
                    neighbors[0] = cellID;
                    break;

				case 1: 
                    RotateAround(ref newNeighbor, Float2.up * 0.5775f, 120, true);
                    break;
				case 2:
                    RotateAround(ref newNeighbor, Float2.up * 0.5775f, -120, true);
                    break;
            }
            
            newNeighbor.Scale = scale;
            newNeighbor.Neighbors = neighbors;
            return newNeighbor;
        }

        public override void Setup()
        {
            symmetryTemplate = new Symmetry3(Float2.zero, Float2.one, 0f);
        }

        public override IEnumerator Hexize()
        {
            int[] vertCounts = new int[vertices.Length];
            List<int> hexVerts = new List<int>(), 
                      openVerts = new List<int>();
            

            int l = symmetries.Length;
            for (int i = 0; i < l; i++)
            {
                vertCounts[symmetries[i].vert1]++;
                vertCounts[symmetries[i].vert2]++;
                vertCounts[symmetries[i].vert3]++;
                
            }

            bool foundStartingHex = false;

            for(int i = 0; i < borderSymmetries.Length; i++)
            {
                int[] v = symmetries[borderSymmetries[i]].Vertices;
                for(int j = 0; j < v.Length; j++)
                {
                    if(vertCounts[v[j]]== 6) 
                    {
                        openVerts.Add(v[j]); 
                        foundStartingHex = true; 
                        break; 
                    }
                }
                if(foundStartingHex) { break; }
            }
            
            yield return null;

            while(openVerts.Count > 0)
            {
                int currentHexCenter = openVerts[0];
                openVerts.RemoveAt(0);
                for(int i = 0; i < vertices.Length; i++)
                {
                    if(i == currentHexCenter || 
                        vertCounts[i] != 6 || 
                        openVerts.Contains(i) || 
                        hexVerts.Contains(i)) { continue; }
                    
                    float d = Float2.Distance(vertices[currentHexCenter], vertices[i]);
                    
                    if(Float.Approximately(d, 1.732f, PositionTolerance))
                    {
                        openVerts.Add(i);
                    }
                }
                hexVerts.Add(currentHexCenter);
                yield return null;
            }
            hexCenters = hexVerts.ToArray();

            

            IEnumerator getHexEdges = GetHexEdges();
            while (getHexEdges.MoveNext()) { yield return null; }

            List<Symmetry6> hexes = new List<Symmetry6>();

            for(int i = 0; i < hexCenters.Length; i++)
            {
                int centerID = hexCenters[i];
                Float2 centerV = vertices[centerID];

                int left = -1, upperLeft = -1, upperRight = -1,
                    right = -1, bottomRight = -1, bottomLeft = -1;

                int leftToUpperLeft = -1, upperLeftToUpperRight = -1, upperRightToRight = -1,
                    rightToBottomRight = -1, bottomRightToBottomLeft = -1, bottomLeftToLeft = -1;

                for (int j = 0; j < symmetries.Length; j++)
                {
                    int[] cellEdges = symmetries[j].Edges;
                    bool cellIsInHex = false;
                    int nonCentralEdge = -1;

                    for(int k = 0; k < cellEdges.Length; k++)
                    {
                        SymmetryEdge cellEdge = edges[cellEdges[k]];
                        if(cellEdge.vertexA == centerID || cellEdge.vertexB == centerID)
                        {
                            cellIsInHex = true;

                        }
                        else
                        {
                            nonCentralEdge = cellEdges[k];
                        }
                       
                    }

                    if (cellIsInHex && nonCentralEdge >=0)
                    {
                        SymmetryEdge nce = edges[nonCentralEdge];
                        Float2 a = vertices[nce.vertexA], b = vertices[nce.vertexB];

                        //Things I know about a hexagon:

                        if (a.y > centerV.y)
                        {
                            if (b.y > centerV.y)
                            {
                                //Top: Both verts of edge are on the same Y and above the center
                                upperLeftToUpperRight = nonCentralEdge;
                                if (a.x < b.x) { upperLeft = nce.vertexA; upperRight = nce.vertexB; }
                                else { upperLeft = nce.vertexB; upperRight = nce.vertexA; }
                            }
                            else if (b.x > centerV.x)
                            {
                                //Upper Right to Right
                                upperRightToRight = nonCentralEdge;
                                upperRight = nce.vertexA; right = nce.vertexB;
                            }
                            else
                            {
                                //Upper Left to Left
                                leftToUpperLeft = nonCentralEdge;
                                upperLeft = nce.vertexA; left = nce.vertexB;
                            }
                        }
                        else if (a.y < centerV.y)
                        {
                            if (b.y < centerV.y)
                            {
                                //Bottom: Both verts of edge are on the same Y and below the center
                                bottomRightToBottomLeft = nonCentralEdge;
                                if (a.x < b.x) { bottomLeft = nce.vertexA; bottomRight = nce.vertexB; }
                                else { bottomLeft = nce.vertexB; bottomRight = nce.vertexA; }
                            }
                            else if (b.x > centerV.x)
                            {
                                //Bottom Right to Right
                                rightToBottomRight = nonCentralEdge;
                                bottomRight = nce.vertexA; right = nce.vertexB;
                            }
                            else
                            {
                                //Bottom Left to Left
                                bottomLeftToLeft = nonCentralEdge;
                                bottomLeft = nce.vertexA; left = nce.vertexB;

                            }

                        }
                        else if(a.x < centerV.x)
                        {
                            left = nce.vertexA;
                            if(b.y > centerV.y)
                            {
                                //Left to Upper Left
                                leftToUpperLeft = nonCentralEdge;
                                upperLeft = nce.vertexB;
                            }
                            else
                            {
                                //Bottom left to Left
                                bottomLeftToLeft = nonCentralEdge;
                                bottomLeft = nce.vertexB;
                            }
                        }
                        else
                        {
                            right = nce.vertexA;
                            if(b.y > centerV.y)
                            {
                                //Upper Right to Right
                                upperRightToRight = nonCentralEdge;
                                upperRight = nce.vertexB;
                            }
                            else
                            {
                                //Right to Bottom Right
                                rightToBottomRight = nonCentralEdge;
                                bottomRight = nce.vertexB;
                            }
                        }
                    }
                }

                //Now we want to convert from triangle edges to hex edges.
                bool setleftToUpperLeft = false,
                     setupperLeftToUpperRight = false,
                     setupperRightToRight = false,
                     setrightToBottomRight = false,
                     setbottomRightToBottomLeft = false,
                     setbottomLeftToLeft = false;

                for (int j = hexEdgeIDs.Length-1; j >=0; j--)
                {
                    
                    if (!setleftToUpperLeft && hexEdgeIDs[j] == leftToUpperLeft)
                    {
                        setleftToUpperLeft = true;
                        leftToUpperLeft = j;
                        SymmetryEdge hexEdge = hexEdges[j];
                        if (hexEdges[j].nodeA < 0)
                        {
                            hexEdge.nodeA = hexes.Count;
                        } 
                        else if(hexEdges[j].nodeB < 0)
                        {
                            hexEdge.nodeB = hexes.Count;
                        }
                        hexEdges[j] = hexEdge;
                    }

                    if (!setupperLeftToUpperRight && hexEdgeIDs[j] == upperLeftToUpperRight) 
                    {
                        setupperLeftToUpperRight = true; 
                        upperLeftToUpperRight = j;
                        SymmetryEdge hexEdge = hexEdges[j];
                        if (hexEdges[j].nodeA < 0)
                        {
                            hexEdge.nodeA = hexes.Count;
                        }
                        else if (hexEdges[j].nodeB < 0)
                        {
                            hexEdge.nodeB = hexes.Count;
                        }
                        hexEdges[j] = hexEdge;
                    }
                    
                    if (!setupperRightToRight && hexEdgeIDs[j] == upperRightToRight)
                    {
                        setupperRightToRight = true;
                        upperRightToRight = j;
                        SymmetryEdge hexEdge = hexEdges[j];
                        if (hexEdges[j].nodeA < 0)
                        {
                            hexEdge.nodeA = hexes.Count;
                        }
                        else if (hexEdges[j].nodeB < 0)
                        {
                            hexEdge.nodeB = hexes.Count;
                        }
                        hexEdges[j] = hexEdge;
                    }
                    
                    if (!setrightToBottomRight && hexEdgeIDs[j] == rightToBottomRight)
                    {
                        setrightToBottomRight = true;
                        rightToBottomRight = j;
                        SymmetryEdge hexEdge = hexEdges[j];
                        if (hexEdges[j].nodeA < 0)
                        {
                            hexEdge.nodeA = hexes.Count;
                        }
                        else if (hexEdges[j].nodeB < 0)
                        {
                            hexEdge.nodeB = hexes.Count;
                        }
                        hexEdges[j] = hexEdge;
                    }
                    
                    if (!setbottomRightToBottomLeft && hexEdgeIDs[j] == bottomRightToBottomLeft)
                    {
                        setbottomRightToBottomLeft = true;
                        bottomRightToBottomLeft = j;
                        SymmetryEdge hexEdge = hexEdges[j];
                        if (hexEdges[j].nodeA < 0)
                        {
                            hexEdge.nodeA = hexes.Count;
                        }
                        else if (hexEdges[j].nodeB < 0)
                        {
                            hexEdge.nodeB = hexes.Count;
                        }
                        hexEdges[j] = hexEdge;
                    }
                    
                    if (!setbottomLeftToLeft && hexEdgeIDs[j] == bottomLeftToLeft)
                    {
                        setbottomLeftToLeft = true;
                        bottomLeftToLeft = j;
                        SymmetryEdge hexEdge = hexEdges[j];
                        if (hexEdges[j].nodeA < 0)
                        {
                            hexEdge.nodeA = hexes.Count;
                        }
                        else if (hexEdges[j].nodeB < 0)
                        {
                            hexEdge.nodeB = hexes.Count;
                        }
                        hexEdges[j] = hexEdge;
                    }
                }
                
                SymmetryNeighbor unsetNeighbor = new SymmetryNeighbor() { direction = Graphs.PortDirection.Closed, neighborID = -1 };
                Symmetry6 newHex = new Symmetry6()
                {
                    position = vertices[centerID],

                    vert1 = left,
                    vert2 = upperLeft,
                    vert3 = upperRight,
                    vert4 = right,
                    vert5 = bottomRight,
                    vert6 = bottomLeft,

                    edge1 = leftToUpperLeft,
                    edge2 = upperLeftToUpperRight,
                    edge3 = upperRightToRight,
                    edge4 = rightToBottomRight,
                    edge5 = bottomRightToBottomLeft,
                    edge6 = bottomLeftToLeft,
                    neighbor1 = unsetNeighbor,
                    neighbor2 = unsetNeighbor,
                    neighbor3 = unsetNeighbor, 
                    neighbor4 = unsetNeighbor,
                    neighbor5 = unsetNeighbor,
                    neighbor6 = unsetNeighbor
                };

                hexes.Add(newHex);
                
                
            }

            yield return null;

            //Neighbor Pairings:
            //edge1 <-> edge4
            //edge2 <-> edge5
            //edge3 <-> edge6

            for (int i = hexCenters.Length-1; i >=1; i--)
            {
                Symmetry6 hexI = hexes[i];
                for (int j = i-1; j>=0; j--)
                {
                    Symmetry6 hexJ = hexes[j];
                    if(hexI.edge1 == hexJ.edge4)
                    {
                        hexI.neighbor1 = j;
                        hexJ.neighbor4 = i;
                    }
                    else if(hexI.edge4 == hexJ.edge1)
                    {
                        hexI.neighbor4 = j;
                        hexJ.neighbor1 = i;
                    }
                    else if (hexI.edge2 == hexJ.edge5)
                    {
                        hexI.neighbor2 = j;
                        hexJ.neighbor5 = i;
                    }
                    else if (hexI.edge5 == hexJ.edge2)
                    {
                        hexI.neighbor5 = j;
                        hexJ.neighbor2 = i;
                    }
                    else if (hexI.edge3 == hexJ.edge6)
                    {
                        hexI.neighbor3 = j;
                        hexJ.neighbor6 = i;
                    }
                    else if (hexI.edge6 == hexJ.edge3)
                    {
                        hexI.neighbor6 = j;
                        hexJ.neighbor3 = i;
                    }
                    hexes[j] = hexJ;
                }
                hexes[i] = hexI;
            }
            hexSymmetries = hexes.ToArray();
            
               
            List<int> toRemove = new List<int>();
            List<int> allHexVerts = new List<int>();
            for (int i = 0; i < hexSymmetries.Length; i++)
            {
                int[] hexers = hexSymmetries[i].Vertices;
                for(int j = 0; j < hexers.Length; j++)
                {
                    if (!allHexVerts.Contains(hexers[j])) { allHexVerts.Add(hexers[j]); }
                }
            }
            for(int i = 0; i < hexCenters.Length; i++)
            {
                if (!allHexVerts.Contains(hexCenters[i])) { allHexVerts.Add(hexCenters[i]); }
            }
            
            for(int i = 0; i < symmetries.Length; i++)
            {
                bool keep = true;
                int[] symVerts = symmetries[i].Vertices;
                for(int j = 0; j < symVerts.Length; j++)
                {
                    if (!allHexVerts.Contains(symVerts[j])) { keep = false; break; }
                }
                if (!keep) { toRemove.Add(i); }
            }

            RemoveSymmetries(toRemove.ToArray());
            

            yield break;
        }
    } 
    
    
}

