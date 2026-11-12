using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Formal.Graphs;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

using UnityEngine;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    public abstract class HexagonalSymmetryGroup<TGroup, TSymmetry> : SymmetryGroup<TGroup,TSymmetry> 
        where TSymmetry : ISymmetry
        where TGroup : HexagonalSymmetryGroup<TGroup, TSymmetry>
    {

        protected static Float2[] neighborDirections = new Float2[]
        {
            new Float2(-1,1),
            new Float2(0, 1),
            new Float2(1,1),
            new Float2(1,-1),
            new Float2(0,-1),
            new Float2(-1,-1)
        };
        
        public override event SymmetryHandler OnRemoveSymmetry;

        public Symmetry6[] hexSymmetries;

        public int[] hexCenters = new int[0], 
                     hexEdgeIDs = new int[0];

        public SymmetryEdge[] hexEdges = new SymmetryEdge[0];

        public virtual int[] HexCenterIDS { get { return hexCenters; } }
        
        public virtual SymmetryEdge[] HexEdges { get { return hexEdges; } }
        
        public virtual int[] HexEdgeIDs { get { return hexEdgeIDs; } }

        public virtual Float2[] HexagonPoints
        {
            get
            {
                Float2[] hexPoints = new Float2[hexCenters.Length];
                for (int i = hexPoints.Length - 1; i >= 0; i--)
                {
                    hexPoints[i] = new Float2(vertices[hexCenters[i]].x, vertices[hexCenters[i]].y);
                }
                return hexPoints;
            }
        }

        #region Constructors
        
        public HexagonalSymmetryGroup() : base()
        { }

        public HexagonalSymmetryGroup(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }
        
        #endregion

        public override IEnumerator BuildGroup(Spline2[] region, Float2 startingPoint, bool requireAllPointsCoverage)
        {
            IEnumerator baseBuild = base.BuildGroup(region, startingPoint, requireAllPointsCoverage);
            while (baseBuild.MoveNext()){ yield return null; }
            
            baseBuild = Hexize();
            while (baseBuild.MoveNext()) { yield return null; }

            yield break;
        }

        public abstract IEnumerator Hexize();

        public virtual IEnumerator GetHexEdges()
        {
            List<int> hedgeIDs = new List<int>();
            List<SymmetryEdge> hedges = new List<SymmetryEdge>();

            for (int i = 0; i < edges.Length; i++)
            {
                bool addThisEdge = true, isPartOfHex = false;
                int[] symVertsA = symmetries[edges[i].nodeA].Vertices,
                      symVertsB = new int[0];
                if(edges[i].nodeB >= 0)
                {
                    symVertsB = symmetries[edges[i].nodeB].Vertices;
                }
                for (int j = hexCenters.Length - 1; j >= 0; j--)
                {
                    if (edges[i].vertexA == hexCenters[j] || edges[i].vertexB == hexCenters[j])
                    {
                        addThisEdge = false;
                        break;
                    }
                    if (!isPartOfHex) 
                    {
                        for (int k = 0; k < symVertsA.Length; k++)
                        {
                            if(symVertsA[k] == hexCenters[j]) { isPartOfHex = true; break; }
                        }
                        if (!isPartOfHex)
                        {
                            for (int k = 0; k < symVertsB.Length; k++)
                            {
                                if (symVertsB[k] == hexCenters[j]) { isPartOfHex = true; break; }
                            }
                        }
                    }    
                }

                if (addThisEdge && isPartOfHex)
                {
                    hedgeIDs.Add(i);
                    hedges.Add(new SymmetryEdge() 
                    {
                         crossDistance = 1, nodeA = -1, nodeB = -1,
                         vertexA = edges[i].vertexA, vertexB = edges[i].vertexB
                    });
                }

                yield return null;
            }
            hexEdgeIDs = hedgeIDs.ToArray();
            hexEdges = hedges.ToArray();
        }

        public override void RemoveSymmetries(int[] ids)
        {
            List<TSymmetry> newSymmetries = new List<TSymmetry>(symmetries);
            List<Symmetry6> newHexSymmetries = new List<Symmetry6>(hexSymmetries);

            int[] newSymmetryIDs = new int[symmetries.Length];
            int[] newHexSymIDs = new int[hexSymmetries.Length];

            List<TSymmetry> removedSyms = new List<TSymmetry>();
            List<Symmetry6> removedHexes = new List<Symmetry6>();

            List<Float2> newPoints = new List<Float2>(vertices);
            int[] newPointIDs = new int[vertices.Length];

            List<SymmetryEdge> newEdges = new List<SymmetryEdge>(edges),
                               newHexEdges = new List<SymmetryEdge>(hexEdges);

            int[] newEdgeIDs = new int[edges.Length],
                  newHexEdgeIDs = new int[hexEdges.Length];

            List<int> pointsToRemove = new List<int>(),
                      edgesToRemove = new List<int>(),
                      hexEdgesToRemove = new List<int>();

            List<int> newHexCenters = new List<int>(hexCenters);

            int c = 0;//, h =0;

            for (int i = 0; i < symmetries.Length; i++)
            {
                bool removeSymmetry = false;

                for (int j = 0; j < ids.Length; j++)
                {
                    if (ids[j] == i)
                    {
                        removeSymmetry = true;
                        break;
                    }
                }

                if (removeSymmetry)
                {
                    newSymmetryIDs[i] = -1;
                    removedSyms.Add(symmetries[i]);
                    newSymmetries.RemoveAt(i - c);
                    c++;
                }
                else
                {
                    newSymmetryIDs[i] = i - c;

                }
            }

            for (int i = removedSyms.Count - 1; i >= 0; i--)
            {
                TSymmetry symmetry = removedSyms[i];

                int[] currentSymmetryVerts = symmetry.Vertices;
                for (int cc = currentSymmetryVerts.Length - 1; cc >= 0; cc--)
                {
                    if (pointsToRemove.Contains(currentSymmetryVerts[cc])) { continue; }

                    bool found = false;
                    for (int oc = newSymmetries.Count - 1; oc >= 0; oc--)
                    {
                        int[] otherSymmetryVerts = newSymmetries[oc].Vertices;
                        for (int ocp = otherSymmetryVerts.Length - 1; ocp >= 0; ocp--)
                        {
                            if (otherSymmetryVerts[ocp] == currentSymmetryVerts[cc])
                            {
                                found = true;
                                break;
                            }
                        }

                        if (found)
                        {
                            break;
                        }
                    }
                    if (!found)
                    {
                        pointsToRemove.Add(currentSymmetryVerts[cc]);
                    }
                }

                if (OnRemoveSymmetry != null)
                {
                    OnRemoveSymmetry(symmetry);
                }
            }

            for (int e = edges.Length - 1; e >= 0; e--)
            {
                if (edgesToRemove.Contains(e)) { continue; }

                for (int i = pointsToRemove.Count - 1; i >= 0; i--)
                {
                    if (pointsToRemove[i] == edges[e].vertexA || pointsToRemove[i] == edges[e].vertexB)
                    {
                        edgesToRemove.Add(e);
                        break;
                    }
                }
            }

            c = 0;

            for (int i = 0; i < vertices.Length; i++)
            {
                bool found = false;
                for (int j = pointsToRemove.Count - 1; j >= 0; j--)
                {
                    if (pointsToRemove[j] == i)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    newPointIDs[i] = i - c;
                }
                else
                {
                    newPointIDs[i] = -1;
                    newPoints.RemoveAt(i - c);
                    c++;
                }
            }

            c = 0;

            for (int i = 0; i < edges.Length; i++)
            {
                bool found = false;

                for (int j = edgesToRemove.Count - 1; j >= 0; j--)
                {
                    if (edgesToRemove[j] == i)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    newEdgeIDs[i] = i - c;
                }
                else
                {

                    newEdgeIDs[i] = -1;
                    newEdges.RemoveAt(i - c);
                    c++;
                }

            }


            //At this point, I have the following:
            //  1. A list of our remaining symmetries and a corresponding list of their original id numbers.
            //  2. A list of our remaining points and a corresponding list of their original ids.
            //  3. A list of our remaining edges and a corresponding list of their original ids.

            // Step 1: Go through our newSymmetries. Update their points by finding that number in the list of old ids. 
            // Update their edgeNeighbors using their original ids.
            for (int i = newSymmetries.Count - 1; i >= 0; i--)
            {
                TSymmetry symmetry = newSymmetries[i];

                int[] ncp = symmetry.Vertices;
                for (int j = ncp.Length - 1; j >= 0; j--)
                {
                    ncp[j] = newPointIDs[ncp[j]];
                }
                symmetry.Vertices = ncp;

                ncp = symmetry.Neighbors;
                for (int j = ncp.Length - 1; j >= 0; j--)
                {
                    if (ncp[j] < 0) { continue; }
                    ncp[j] = newSymmetryIDs[ncp[j]];
                }
                symmetry.Neighbors = ncp;

                ncp = symmetry.Edges;
                for (int j = ncp.Length - 1; j >= 0; j--)
                {
                    if (ncp[j] < 0) { continue; }
                    ncp[j] = newEdgeIDs[ncp[j]];
                }
                symmetry.Edges = ncp;

                newSymmetries[i] = symmetry;
            }

            for (int i = newEdges.Count - 1; i >= 0; i--)
            {

                SymmetryEdge t = new SymmetryEdge();
                t.vertexA = newPointIDs[newEdges[i].vertexA];
                t.vertexB = newPointIDs[newEdges[i].vertexB];
                if (newEdges[i].nodeA >= 0) { t.nodeA = newSymmetryIDs[newEdges[i].nodeA]; } else { t.nodeA = -1; }
                if (newEdges[i].nodeB >= 0) { t.nodeB = newSymmetryIDs[newEdges[i].nodeB]; } else { t.nodeB = -1; }

                // 2. Find out if its a border edge or not.
                // If it WAS a border, and we're removing symmetries, it's STILL a border.
                // So we only check to find out if it has BECOME A border.
                int l = 0;
                for (int j = newSymmetries.Count - 1; j >= 0; j--)
                {
                    if (l > 1) { break; }
                    for (int k = newSymmetries[j].Edges.Length - 1; k >= 0; k--)
                    {
                        if (newSymmetries[j].Edges[k] == i)
                        {
                            l++; break;
                        }
                    }
                }
                if (l == 1)
                {
                    t.isBorder = true;
                }

                newEdges[i] = t;
            }

            for (int i = newHexCenters.Count - 1; i >= 0; i--)
            {
                newHexCenters[i] = newPointIDs[newHexCenters[i]];
                
                if (newHexCenters[i] < 0) { newHexCenters.RemoveAt(i); continue; }

                Symmetry6 hex = hexSymmetries[i];
                int[] hvid = hex.Vertices;
                for (int j = hvid.Length - 1; j >= 0; j--)
                {
                    hvid[j] = newPointIDs[hvid[j]];
                }
                hex.Vertices = hvid;
                hexSymmetries[i] = hex;

            }

            symmetries = newSymmetries.ToArray();
            vertices = newPoints.ToArray();
            edges = newEdges.ToArray();
            hexCenters = newHexCenters.ToArray();
        }

        public virtual bool HexContainsPoint(Float2 p, ref int hexID)
        {
            int symmetryID = -1;
            if (ContainsPoint(p, ref symmetryID))
            {
                //I know the symmetry that contains it. First, I want to find which of its points is a hex center.
                int[] symmetryVerts = symmetries[symmetryID].Vertices;
                for (int i = symmetryVerts.Length - 1; i >= 0; i--)
                {
                    for (int j = hexCenters.Length - 1; j >= 0; j--)
                    {
                        if (symmetryVerts[i] == hexCenters[j])
                        {
                            hexID = j;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        public override void Offset(Float2 offset)
        {
            for (int i = vertices.Length - 1; i >= 0; i--)
            {
                vertices[i] += offset;
            }
            for (int i = symmetries.Length - 1; i >= 0; i--)
            {
                symmetries[i].Position = symmetries[i].Position + offset;
            }

            for (int i = hexSymmetries.Length - 1; i >= 0; i--)
            {
                hexSymmetries[i].Position = hexSymmetries[i].Position + offset;
            }
        }

        public override void Scale(Float2 scalar)
        {
            for (int i = vertices.Length - 1; i >= 0; i--)
            {
                vertices[i] *= scalar;
            }
            for (int i = symmetries.Length - 1; i >= 0; i--)
            {
                symmetries[i].Position = symmetries[i].Position * scalar;
            }

            for (int i = hexSymmetries.Length - 1; i >= 0; i--)
            {
                hexSymmetries[i].Position = hexSymmetries[i].Position * scalar;
            }
        }

        public override int[] GraphSearch(int startSymmetry, int endSymmetry)
        {

            GraphStep startingStep = new GraphStep()
            {
                nodeID = startSymmetry,
                parentID = -1,
                distanceToStep = 0,
                heuristicCost = Float2.ManhattanDistance(hexSymmetries[startSymmetry].Position, hexSymmetries[endSymmetry].Position)
            };
            
            List<GraphStep> openList = new List<GraphStep>();
            openList.Add(startingStep);

            List<GraphStep> closedList = new List<GraphStep>();

            while (openList.Count > 0)
            {
                float lowestCost = float.MaxValue;
                int lowestID = -1;

                for (int i = openList.Count - 1; i >= 0; i--)
                {
                    float openCost = openList[i].StepCost;
                    if (openCost < lowestCost)
                    {
                        lowestCost = openCost; lowestID = i;
                    }
                }

                GraphStep current = openList[lowestID];

                
                var currentSymmetry = hexSymmetries[current.nodeID];

                openList.RemoveAt(lowestID);
                closedList.Add(current);

                int[] neighbors = currentSymmetry.Neighbors;
                int[] neighborEdges = currentSymmetry.Edges;
                for (int j = 0; j < neighbors.Length; j++)
                {
                    int neighborID = neighbors[j];
                    if (neighborID < 0) continue;

                    GraphStep neighborStep = new GraphStep()
                    {
                        parentID = closedList.Count-1,
                        nodeID = neighborID,
                        distanceToStep = current.distanceToStep + hexEdges[neighborEdges[j]].crossDistance,
                        heuristicCost = Float2.ManhattanDistance(hexSymmetries[neighborID].Position, hexSymmetries[endSymmetry].Position)
                    };

                    if (neighborID == endSymmetry)
                    {
                        //Stop search, finalize.
                        List<int> finalList = new List<int>();
                        current = neighborStep;
                        while (current.parentID >= 0)
                        {
                            finalList.Add(current.nodeID);
                            current = closedList[current.parentID];
                        }
                        finalList.Add(startSymmetry);
                        finalList.Reverse();
                        return finalList.ToArray();
                    }

                    bool alreadyInTheOpen = false;
                    for (int k = openList.Count - 1; k >= 0; k--)
                    {
                        if (openList[k].nodeID == neighborID &&
                        openList[k].StepCost < neighborStep.StepCost)
                        {
                            alreadyInTheOpen = true;
                            break;
                        }
                    }

                    if (!alreadyInTheOpen)
                    {
                        bool alreadyClosed = false;
                        for (int k = closedList.Count - 1; k >= 0; k--)
                        {
                            if (closedList[k].nodeID == neighborID)
                            {
                                alreadyClosed = true;
                                if (closedList[k].StepCost > neighborStep.StepCost)
                                {
                                    closedList[k] = neighborStep;
                                    break;
                                }
                            }
                        }
                        if (!alreadyClosed) { openList.Add(neighborStep); }
                    }
                }
            }
            return new int[0];
        }
    }

}
