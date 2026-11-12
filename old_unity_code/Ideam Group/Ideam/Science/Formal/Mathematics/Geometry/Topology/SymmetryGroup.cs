using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Formal.Graphs;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

using UnityEngine;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    public enum SymmetryGroupBuildPhase
    {
        Setup,
        Coverage,
        EdgeDetection,
        BorderDetection,
        Complete
    }
    
    public abstract class SymmetryGroup<TGroup, TSymmetry> : IEquatable<TGroup>, ISymmetryGroup<TSymmetry>, IGraph 
        where TSymmetry : ISymmetry
        where TGroup : SymmetryGroup<TGroup, TSymmetry>
    {
        public XYToXYZ axisConversion;

        struct GenerationSorter
        {
            public int symID;
            public int generation;

            public GenerationSorter(int cid, int g)
            {
                symID = cid;
                generation = g;
            }
        }

        #region IGraph Implementation

        #region Nodes
        public int NodesCount { get { return symmetries.Length; } }
        public virtual T GetNode<T>(int id) where T : INode
        {
            if(symmetries[id] is T tNode) { return tNode; }
            return default;
        }

        public virtual T[] GetNodes<T>() where T : INode
        {
            if (typeof(T).IsAssignableFrom(typeof(TSymmetry)))
            {
                return symmetries as T[];
            }
            return new T[0];
        }

        public virtual int AddNode<T>(T node) where T : INode
        {
            int id = -1;
            if(node is TSymmetry symmetry)
            {
                List<TSymmetry> newSyms = new List<TSymmetry>(symmetries);
                id = newSyms.IndexOf(symmetry);
                if(id < 0) { id = newSyms.Count; newSyms.Add(symmetry); symmetries = newSyms.ToArray(); }
            }
            return id;
        }

        public virtual void RemoveNode(int localID)
        {

        }

        #endregion

        #region Edges
        public int EdgesCount { get { return edges.Length; } }
        public virtual T GetEdge<T>(int id) where T : IEdge
        {
            if (edges[id] is T tEdge) { return tEdge; }
            return default;
        }

        public virtual T[] GetEdges<T>() where T : IEdge
        {
            if (typeof(T).IsAssignableFrom(typeof(SymmetryEdge)))
            {
                return edges as T[];
            }
            return new T[0];
        }

        public virtual int AddEdge<T>(T edge) where T : IEdge
        {
            int id = -1;
            if (edge is SymmetryEdge sEdge)
            {
                List<SymmetryEdge> newEdges = new List<SymmetryEdge>(edges);
                id = newEdges.IndexOf(sEdge);
                if (id < 0) { id = newEdges.Count; newEdges.Add(sEdge); edges = newEdges.ToArray(); }
            }
            return id;
        }

        public virtual void RemoveEdge(int localID)
        {

        }

        #endregion 

        public virtual int[] GraphSearch(int startNode, int endNode)
        {

            GraphStep startingStep = new GraphStep()
            {
                nodeID = startNode,
                parentID = -1,
                distanceToStep = 0,
                heuristicCost = Float2.ManhattanDistance(symmetries[startNode].Position, symmetries[endNode].Position)
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


                var currentSymmetry = symmetries[current.nodeID];

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
                        parentID = closedList.Count - 1,
                        nodeID = neighborID,
                        distanceToStep = current.distanceToStep + edges[neighborEdges[j]].crossDistance,
                        heuristicCost = Float2.ManhattanDistance(symmetries[neighborID].Position, symmetries[endNode].Position)
                    };

                    if (neighborID == endNode)
                    {
                        //Stop search, finalize.
                        List<int> finalList = new List<int>();
                        current = neighborStep;
                        while (current.parentID >= 0)
                        {
                            finalList.Add(current.nodeID);
                            current = closedList[current.parentID];
                        }
                        finalList.Add(startNode);
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

        #endregion

        #region Event Base

        public delegate void SymmetryGroupHandler(TGroup group);
        public delegate void SymmetryHandler(TSymmetry symmetry);

        #endregion

        #region Variables

        public TSymmetry symmetryTemplate;

        public TSymmetry Template { get { return symmetryTemplate; } }

        public TSymmetry[] symmetries = new TSymmetry[0];

        public Float2[] vertices = new Float2[0];

        public SymmetryEdge[] edges = new SymmetryEdge[0];

        protected bool updateBorders = true;

        public int[] borderSymmetries = new int[0], borderEdges, borderVertices;

        public int[] tris = new int[0];

        public Bounds2 extents;

        public Spline2[] outline = new Spline2[0];

        public SymmetryGroupBuildPhase buildPhase;
        public virtual event Action<int> OnCoverageStep;
        public virtual event SymmetryHandler OnRemoveSymmetry, OnAddSymmetry;

        public int[] orphanEdges, impossibleEdges;

        #endregion

        #region Properties

        public virtual TSymmetry[] Symmetries { get { return symmetries; } }
        public virtual Float2[] Vertices { get { return vertices; } }
        public virtual SymmetryEdge[] Edges { get { return edges; } }
        public virtual int[] Triangles
        {
            get
            {
                if (tris.Length < symmetries.Length)
                {
                    List<int> newTris = new List<int>();
                    for (int i = 0; i < symmetries.Length; i++)
                    {
                        newTris.AddRange(symmetries[i].Triangles);
                    }
                    tris = newTris.ToArray();
                }
                return tris;
            }
        }

        public virtual int[] BorderSymmetries
        {
            get
            {
                return borderSymmetries;
            }
        }
        public virtual int[] BorderEdges
        {
            get
            {
                return borderEdges;
            }
        }
        public virtual int[] BorderVertices
        {
            get
            {
                return borderVertices;
            }
        }

        public virtual Bounds2 Extents
        {
            get
            {
                if (extents == default)
                {
                    extents = new Bounds2(vertices[vertices.Length - 1], Float2.zero);
                    for (int i = vertices.Length - 1; i >= 0; i--)
                    {
                        extents.Encapsulate(vertices[i]);
                    }
                }
                return extents;
            }
        }

        public virtual bool GroupIsDirty { get; protected set; }

        public abstract float PositionTolerance { get; set; }
        public abstract float RotationTolerance { get; set; }
        public abstract float ScaleTolerance { get; set; }
        public abstract Float2[] PrimitiveVertices { get; }

        public virtual Spline2[] Outline
        {
            get
            {
                return outline;
            }
        }

        #endregion

        #region Methods

        #region Constructors

        public SymmetryGroup()
        {
            symmetries = new TSymmetry[0];
            vertices = new Float2[0];
            edges = new SymmetryEdge[0];
        }

        public SymmetryGroup(Spline2[] region, Float2 start, Action<IEnumerator> buildAction)
        {
            IEnumerator groupBuilder = BuildGroup(region, start, true);
            buildAction?.Invoke(groupBuilder);
        }

        #endregion

        public abstract void Setup();

        public abstract TSymmetry GetNeighbor(TSymmetry baseSymmetry, int symmetryID, int neighborID);

        public event Action<SymmetryGroup<TGroup,TSymmetry>> OnBuild;

        public virtual IEnumerator BuildGroup(Spline2[] region, Float2 startingPoint, bool requireAllPointsCoverage)
        {
            Setup();

            if (region.Length == 0) { Debug.Log("Error, No Regions."); yield break; }

            Bounds2 regionB = region.GetExtents();
            if (!regionB.Contains(startingPoint)) { Debug.Log("Error, region does not contain starting point."); yield break; }

            OnBuild?.Invoke(this);
            buildPhase = SymmetryGroupBuildPhase.Coverage;

            IEnumerator buildProcess = CoverRegion(region, startingPoint, regionB, requireAllPointsCoverage);
            while (buildProcess.MoveNext()) { yield return null; }


            buildPhase = SymmetryGroupBuildPhase.BorderDetection;

            buildProcess = FindBorders(regionB);
            while (buildProcess.MoveNext()) { yield return null; }


            buildPhase = SymmetryGroupBuildPhase.Complete;

            yield break;

        }

        public virtual IEnumerator CoverRegion(Spline2[] region, Float2 startingPoint, Bounds2 regionBounds, bool allPointsCoverage)
        {
            
            Float2 regionMin = regionBounds.Min;
            Float2 regionMax = regionBounds.Max;

            List<Float2> vertsList = new List<Float2>();
            
            List<TSymmetry> openSymmetries = new List<TSymmetry>(),
                            closedSymmetries = new List<TSymmetry>();

            List<SymmetryEdge> edgesList = new List<SymmetryEdge>();
            
            TSymmetry startingSymmetry = symmetryTemplate;
            startingSymmetry.Position = startingPoint;


            Float2[] startingVerts = GenerateSymmetryVertices(startingSymmetry);
            Float2 minSize = new Float2(startingPoint), maxSize = new Float2(startingPoint);

            int[] ids = startingSymmetry.Vertices;

            for (int i = 0; i < startingVerts.Length; i++)
            {

                ids[i] = vertsList.Count;
                vertsList.Add(startingVerts[i]);
                if (minSize.x < startingVerts[i].x)
                {
                    minSize.x = startingVerts[i].x;
                }
                if (maxSize.x > startingVerts[i].x)
                {
                    maxSize.x = startingVerts[i].x;
                }

                if (minSize.y < startingVerts[i].y)
                {
                    minSize.y = startingVerts[i].y;
                }
                if (maxSize.y > startingVerts[i].y)
                {
                    maxSize.y = startingVerts[i].y;
                }
            }

            Float2 symmetrySize = (minSize - maxSize) * 0.5f;
            symmetrySize = new Float2(Float.Abs(symmetrySize.x), Float.Abs(symmetrySize.y));

            startingSymmetry.Vertices = ids;

            SymmetryEdge[] firstEdges = GenerateSymmetryEdges(startingSymmetry, 0);
            edgesList.AddRange(firstEdges);
            ids = startingSymmetry.Edges;
            for(int i =0; i < firstEdges.Length; i++)
            {
                ids[i] = i;
            }
            startingSymmetry.Edges = ids;

            ids = startingSymmetry.Neighbors;
            for (int i = ids.Length - 1; i >= 0; i--)
            {
                ids[i] = -1;
            }
            startingSymmetry.Neighbors = ids;

            openSymmetries.Add(startingSymmetry);
            

            while (openSymmetries.Count > 0)
            {
                TSymmetry currentSymmetry = openSymmetries[0];
                openSymmetries.RemoveAt(0);

                int currentID = closedSymmetries.Count;

                int[] neighbors = currentSymmetry.Neighbors;

                for (int j = 0; j < neighbors.Length; j++)
                {
                    if (neighbors[j] == -1)
                    {
                        TSymmetry neighborSymmetry = GetNeighbor(currentSymmetry, currentID, j);

                        bool neighborInClosedSet = false;

                        for (int k = closedSymmetries.Count - 1; k >= 0; k--)
                        {
                            if (WithinTolerance(neighborSymmetry, closedSymmetries[k]))
                            {
                                neighborInClosedSet = true;
                                neighbors[j] = k;
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
                                    neighbors[j] = currentID + k;
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
                                            neighbors[j] = -2;
                                            addSym = false;
                                            break;
                                        }
                                    }
                                }
                                if (!addSym) { continue; }
                                neighbors[j] = currentID + openSymmetries.Count;

                                int[] neighborVertIDs = neighborSymmetry.Vertices;

                                for (int k = 0; k < neighborVertices.Length; k++)
                                {
                                    bool hasPoint = false;

                                    for (int l = vertsList.Count - 1; l >= 0; l--)
                                    {
                                        if (neighborVertices[k].NearlyEqual(vertsList[l], PositionTolerance))
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
                                SymmetryEdge[] symEdges = GenerateSymmetryEdges(neighborSymmetry, neighbors[j]);
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
                                            nE.nodeB = neighbors[j];
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
                                neighbors[j] = -2;
                            }
                        }
                    }
                }

                currentSymmetry.Neighbors = neighbors;
                closedSymmetries.Add(currentSymmetry);
                OnAddSymmetry?.Invoke(currentSymmetry);
                OnCoverageStep?.Invoke(closedSymmetries.Count);
                yield return null;
            }

            /*
            //Stabalize Vert positions
            List<float> stabalizedXs = new List<float>(),
                        stabalizedYs = new List<float>();

            for (int i = 0; i < vertsList.Count; i++)
            {
                bool addVal = true;
                float x = vertsList[i].x;

                for (int sx = 0; sx < stabalizedXs.Count; sx++)
                {
                    if (Float.Approximately(x, stabalizedXs[sx], PositionTolerance))
                    {
                        x = stabalizedXs[sx];
                        addVal = false;
                        break;
                    }
                }
                if (addVal) { stabalizedXs.Add(vertsList[i].x); }

                addVal = true;
                float y = vertsList[i].y;

                for (int sy = 0; sy < stabalizedYs.Count; sy++)
                {
                    if (Float.Approximately(y, stabalizedYs[sy], PositionTolerance))
                    {
                        y = stabalizedYs[sy];
                        addVal = false;
                        break;
                    }
                }
                if (addVal) { stabalizedYs.Add(vertsList[i].y); }
                vertsList[i] = new Float2(x, y);
            }
            */

            symmetries = closedSymmetries.ToArray();
            vertices = vertsList.ToArray();
            
            edges = edgesList.ToArray();

            yield break;
        }

        public virtual IEnumerator FindBorders(Bounds2 regionBounds)
        {
            List<int> orphansList = new List<int>(),
            impossiblesList = new List<int>(),
            outlineIDsList = new List<int>(),
            borderSymsList = new List<int>(),
            borderEdgesList = new List<int>(),
            borderVertsList = new List<int>();

            List<SymmetryEdge> outlineEdgesList = new List<SymmetryEdge>();
            List<Spline2> outlinesList = new List<Spline2>();

            int[] edgeUseCounts = new int[edges.Length];
            List<int>[] symIDs = new List<int>[edges.Length];

            for (int i = 0; i < symmetries.Length; i++)
            {
                TSymmetry sym = symmetries[i];
                sym.IsOnBorder = false;

                int[] symEdgeIDs = sym.Edges;
                for (int j = symEdgeIDs.Length - 1; j >= 0; j--)
                {
                    if (symEdgeIDs[j] >= 0)
                    {
                        edgeUseCounts[symEdgeIDs[j]]++;
                        if (symIDs[symEdgeIDs[j]] == null)
                        {
                            symIDs[symEdgeIDs[j]] = new List<int>();
                        }
                        symIDs[symEdgeIDs[j]].Add(i);
                    }
                }
                symmetries[i] = sym;
            }


            int symID;

            for (int i = 0; i < edgeUseCounts.Length; i++)
            {
                SymmetryEdge edge = edges[i];
                switch (edgeUseCounts[i])
                {
                    case 0:
                        orphansList.Add(i);
                        break;
                    case 1:
                        edge.isBorder = true;

                        symID = symIDs[i][0];
                        edge.nodeA = symID;
                        edge.nodeB = -1;
                        TSymmetry sym = symmetries[symID];
                        sym.IsOnBorder = true;
                        symmetries[symID] = sym;
                        break;
                    case 2:
                        edge.isBorder = false;
                        symID = symIDs[i][0];
                        edge.nodeA = symID;
                        symID = symIDs[i][1];
                        edge.nodeB = symID;
                        break;
                    default:
                        impossiblesList.Add(i);
                        break;
                }
                edges[i] = edge;
            }

            orphanEdges = orphansList.ToArray();
            impossibleEdges = impossiblesList.ToArray();

            for (int i = edges.Length - 1; i >= 0; i--)
            {
                if (edges[i].isBorder)
                {
                    SymmetryEdge outlineEdge = new SymmetryEdge(edges[i]);

                    outlineEdgesList.Add(outlineEdge);
                    outlineIDsList.Add(i);
                }
            }

            yield return null;

            Float2 sidStart = new Float2(regionBounds.Max.x,regionBounds.Min.y);
            
            int startEdgeID = -1, endEdgeID = -1;
            while (outlineEdgesList.Count > 2)
            {
                startEdgeID = endEdgeID = -1;

                List<Float2> thisShape = new List<Float2>();
                List<int> symmetryIDs = new List<int>(),
                          edgeIDs = new List<int>(),
                          vertexIDs = new List<int>();

                int symmetryID = -1, edgeID = -1, pointID = -1, sid = -1;

                for (int i = outlineEdgesList.Count - 1; i >= 0; i--)
                {

                    SymmetryEdge current = outlineEdgesList[i];

                    //Find the most upper leftist.
                    Float2 workVec;

                    workVec = vertices[current.vertexA];
                    if (workVec.x <= sidStart.x && workVec.y > sidStart.y)
                    {
                        sidStart = workVec;
                        sid = current.vertexA;
                        symmetryID = current.nodeA;
                        edgeID = i;
                        pointID = sid;
                    }

                    workVec = vertices[current.vertexB];
                    if (workVec.x <= sidStart.x && workVec.y > sidStart.y)
                    {

                        sidStart = workVec;
                        sid = current.vertexB;
                        symmetryID = current.nodeA;
                        edgeID = i;
                        pointID = sid;
                    }
                    
                }
                
                if(sid < 0) { Debug.Log("Border Error: " + outlineEdgesList.Count + " edges left, with no starting point found."); outlineEdgesList.Clear(); continue; }

                Float2 currentPoint = vertices[sid];
                
                thisShape.Add(currentPoint);
                symmetryIDs.Add(symmetryID);
                vertexIDs.Add(pointID);
                edgeIDs.Add(edgeID);

                //1. Find our startEdge and EndEdge. They both make use of our starting point.
                for (int i = outlineEdgesList.Count - 1; i >= 0; i--)
                {
                    if (outlineEdgesList[i].vertexA == sid || outlineEdgesList[i].vertexB == sid)
                    {
                        if (startEdgeID == -1)
                        {
                            startEdgeID = i;
                        }
                        else if (endEdgeID == -1)
                        {
                            endEdgeID = i;
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                if (startEdgeID == -1 || endEdgeID == -1) { break; }

                Float2 startA = vertices[outlineEdgesList[startEdgeID].vertexA],
                endA = vertices[outlineEdgesList[endEdgeID].vertexA],
                startB = vertices[outlineEdgesList[startEdgeID].vertexB],
                endB = vertices[outlineEdgesList[endEdgeID].vertexB],
                diff1, diff2;

                if (startA != currentPoint)
                {
                    if (endA != currentPoint)
                    {
                        //current, sa, ea
                        diff1 = startA - currentPoint;
                        diff2 = endA - currentPoint;
                    }
                    else
                    {
                        //current, sa, eb
                        diff1 = startA - currentPoint;
                        diff2 = endB - currentPoint;
                    }
                }
                else
                {
                    if (endA != currentPoint)
                    {
                        //current, sb, ea
                        diff1 = startB - currentPoint;
                        diff2 = endA - currentPoint;
                    }
                    else
                    {
                        //current, sb, eb
                        diff1 = startB - currentPoint;
                        diff2 = endB - currentPoint;
                    }
                }

                if (diff2.x >= diff1.x && diff2.y > diff1.y)
                {
                    //swap!
                    int swapper = startEdgeID;
                    startEdgeID = endEdgeID;
                    endEdgeID = swapper;
                }

                int currentEdge = startEdgeID;

                //Then we just go through all our remaining closedEdges, point to point, until we arrive back at end. Each time we complete an edge, we remove it from consideration.
                while (currentEdge != endEdgeID)
                {
                    //First, find the point that's NOT currentPoint. Add it to our pointsList.
                    if (vertices[outlineEdgesList[currentEdge].vertexA] != currentPoint)
                    {
                        currentPoint = vertices[outlineEdgesList[currentEdge].vertexA];
                        thisShape.Add(currentPoint);
                        vertexIDs.Add(outlineEdgesList[currentEdge].vertexA);
                    }
                    else
                    {
                        currentPoint = vertices[outlineEdgesList[currentEdge].vertexB];
                        thisShape.Add(currentPoint);
                        vertexIDs.Add(outlineEdgesList[currentEdge].vertexB);
                    }

                    //Next, add our border IDs to our lists.
                    symmetryIDs.Add(outlineEdgesList[currentEdge].nodeA);
                    edgeIDs.Add(outlineIDsList[currentEdge]);

                    //Then, remove that Edge.
                    outlineEdgesList.RemoveAt(currentEdge);
                    outlineIDsList.RemoveAt(currentEdge);

                    if (endEdgeID > currentEdge)
                    {
                        endEdgeID--;
                    }
                    //Last, find our next edge.
                    for (int i = outlineEdgesList.Count - 1; i >= 0; i--)
                    {
                        //Find another that has currentPoint. That's our next link.
                        if (vertices[outlineEdgesList[i].vertexA].Equals(currentPoint) ||
                            vertices[outlineEdgesList[i].vertexB].Equals(currentPoint))
                        {
                            currentEdge = i;
                        }
                    }
                }

                borderSymsList.AddRange(symmetryIDs);
                borderSymsList.Add(-1);

                borderEdgesList.AddRange(edgeIDs);
                borderEdgesList.Add(-1);

                borderVertsList.AddRange(vertexIDs);
                borderVertsList.Add(-1);
                outlinesList.Add(new Spline2(thisShape.ToArray()));

                yield return null;
                //When we've arrived back at the start, check to see if we've still got unused closedEdges. They're part of another shape.
            }


            borderSymmetries = borderSymsList.ToArray();
            borderEdges = borderEdgesList.ToArray();
            borderVertices = borderVertsList.ToArray();
            outline = outlinesList.ToArray();

            yield break;
        }


        public virtual void Clear()
        {
            symmetries = new TSymmetry[0];
            vertices = new Float2[0];
            edges = new SymmetryEdge[0];
            tris = new int[0];

            borderVertices = new int[0];
            borderSymmetries = new int[0];
            borderEdges = new int[0];
        }

        
        public int[] GetSymmetryIDsByGeneration()
        {
            List<GenerationSorter> sorter = new List<GenerationSorter>(symmetries.Length);
            for (int i = symmetries.Length - 1; i >= 0; i--)
            {
                sorter[i] = new GenerationSorter(i, symmetries[i].Generation);
            }
            sorter.Sort((x, y) => x.generation.CompareTo(y.generation));

            List<int> symsByGeneration = new List<int>(symmetries.Length);
            for (int i = sorter.Count - 1; i >= 0; i--)
            {
                symsByGeneration[i] = sorter[i].symID;

            }

            return symsByGeneration.ToArray();
        }

        public TSymmetry[] GetSymmetriesByGeneration()
        {
            List<TSymmetry> byGen = new List<TSymmetry>(symmetries);
            byGen.Sort((x, y) => x.Generation.CompareTo(y.Generation));
            return byGen.ToArray();
        }


        public virtual bool ContainsPoint(Float2 p, ref int id)
        {
            int i, j;
            for (i = symmetries.Length - 1; i >= 0; i--)
            {
                int[] pts = symmetries[i].Vertices;
                bool inside = false;
                j = pts.Length - 1;
                for (int k = 0; k < pts.Length; j = k++)
                {
                    Float2 pi = vertices[pts[k]];
                    Float2 pj = vertices[pts[j]];

                    if (((pi.y <= p.y && p.y < pj.y) || (pj.y <= p.y && p.y < pi.y)) &&
                        (p.x < (pj.x - pi.x) * (p.y - pi.y) / (pj.y - pi.y) + pi.x))
                    {
                        inside = !inside;
                    }
                }

                if (inside) { id = i; return true; }
            }
            id = -1;
            return false;
        }

        public virtual Bounds2 GetSymmetryBounds(int id)
        {

            Bounds2 symBounds = new Bounds2();
            if (id < 0 || id > symmetries.Length) { return symBounds; }

            int[] cp = symmetries[id].Vertices;

            for (int i = cp.Length - 1; i >= 0; i--)
            {
                symBounds.Encapsulate(vertices[cp[i]]);
            }

            return symBounds;
        }

        public virtual Float2[] GetSymmetryVertices(int id)
        {
            if (id < 0 || id >= symmetries.Length) { return new Float2[0]; }

            int[] vertIDs = symmetries[id].Vertices;

            Float2[] symmVerts = new Float2[vertIDs.Length];

            for (int i = vertIDs.Length - 1; i >= 0; i--)
            {
                symmVerts[i] = new Float2(vertices[vertIDs[i]].x, vertices[vertIDs[i]].y);
            }

            return symmVerts;
        }
        
        public virtual Float2[] GenerateSymmetryVertices(TSymmetry symmetry)
        {
            return GenerateSymmetryVertices(symmetry, PrimitiveVertices);
        }

        public virtual Float2[] GenerateSymmetryVertices(TSymmetry symmetry, Float2[] points)
        {
            Matrix3x3 trans = Matrix3x3.GlobalToLocalMatrix(symmetry.Angle, symmetry.Scale, symmetry.Position);

            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i] = points[i] * trans;
            }

            return points;
        }

        public virtual SymmetryEdge[] GetSymmetryEdges(int id)
        {
            int[] edgeIDs = symmetries[id].Edges;

            SymmetryEdge[] symmEdges = new SymmetryEdge[edgeIDs.Length];
            for (int i = edgeIDs.Length - 1; i >= 0; i--)
            {
                symmEdges[i] = new SymmetryEdge(edges[edgeIDs[i]]);
            }
            return symmEdges;
        }

        public virtual SymmetryEdge[] GenerateSymmetryEdges(TSymmetry symmetry, int id)
        {
            int[] vertIDs = symmetry.Vertices;

            SymmetryEdge[] symmEdges = new SymmetryEdge[vertIDs.Length];
            for (int i = 0; i < vertIDs.Length - 1; i++)
            {
                symmEdges[i] = new SymmetryEdge(vertIDs[i], vertIDs[i + 1]);
                symmEdges[i].nodeA = id;
            }
            symmEdges[vertIDs.Length - 1] = new SymmetryEdge(vertIDs[vertIDs.Length - 1], vertIDs[0]);
            return symmEdges;
        }

        
        public virtual void RemoveSymmetries(int[] ids)
        {
            List<TSymmetry> newSymmetries = new List<TSymmetry>(symmetries);
            int[] newSymmIDs = new int[symmetries.Length];
            List<TSymmetry> removedSymmetries = new List<TSymmetry>();

            List<Float2> newPoints = new List<Float2>(vertices);
            int[] newPointIDs = new int[vertices.Length];

            List<SymmetryEdge> newEdges = new List<SymmetryEdge>(edges);
            int[] newEdgeIDs = new int[edges.Length];

            List<int> pointsToRemove = new List<int>();
            List<int> edgesToRemove = new List<int>();

            int c = 0;

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
                    newSymmIDs[i] = -1;
                    removedSymmetries.Add(symmetries[i]);
                    newSymmetries.RemoveAt(i - c);
                    c++;
                }
                else
                {
                    newSymmIDs[i] = i - c;

                }
            }

            for (int i = removedSymmetries.Count - 1; i >= 0; i--)
            {
                TSymmetry symm = removedSymmetries[i];

                int[] currentSymVerts = symm.Vertices;
                for (int cc = currentSymVerts.Length - 1; cc >= 0; cc--)
                {
                    if (pointsToRemove.Contains(currentSymVerts[cc])) { continue; }

                    bool found = false;
                    for (int oc = newSymmetries.Count - 1; oc >= 0; oc--)
                    {
                        int[] otherSymVerts = newSymmetries[oc].Vertices;
                        for (int ocp = otherSymVerts.Length - 1; ocp >= 0; ocp--)
                        {
                            if (otherSymVerts[ocp] == currentSymVerts[cc])
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
                        pointsToRemove.Add(currentSymVerts[cc]);
                    }
                }

                if (OnRemoveSymmetry != null)
                {
                    OnRemoveSymmetry(symm);
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
                TSymmetry symm = newSymmetries[i];

                int[] ncp = symm.Vertices;
                for (int j = ncp.Length - 1; j >= 0; j--)
                {
                    ncp[j] = newPointIDs[ncp[j]];
                }
                symm.Vertices = ncp;

                ncp = symm.Neighbors;
                for (int j = ncp.Length - 1; j >= 0; j--)
                {
                    if (ncp[j] < 0) { continue; }
                    ncp[j] = newSymmIDs[ncp[j]];
                }
                symm.Neighbors = ncp;

                ncp = symm.Edges;
                for (int j = ncp.Length - 1; j >= 0; j--)
                {
                    if (ncp[j] < 0) { continue; }
                    ncp[j] = newEdgeIDs[ncp[j]];
                }
                symm.Edges = ncp;

                newSymmetries[i] = symm;
            }

            for (int i = newEdges.Count - 1; i >= 0; i--)
            {

                SymmetryEdge symmEdge = new SymmetryEdge();
                symmEdge.vertexA = newPointIDs[newEdges[i].vertexA];
                symmEdge.vertexB = newPointIDs[newEdges[i].vertexB];
                if (newEdges[i].nodeA >= 0) { symmEdge.nodeA = newSymmIDs[newEdges[i].nodeA]; } else { symmEdge.nodeA = -1; }
                if (newEdges[i].nodeB >= 0) { symmEdge.nodeB = newSymmIDs[newEdges[i].nodeB]; } else { symmEdge.nodeB = -1; }

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
                    symmEdge.isBorder = true;
                }

                newEdges[i] = symmEdge;
            }

            symmetries = newSymmetries.ToArray();
            vertices = newPoints.ToArray();
            edges = newEdges.ToArray();
        }

        public virtual bool WithinTolerance(TSymmetry a, TSymmetry b)
        {
            Float2 diff = a.Position - b.Position;
            if (Float.Abs(diff.x) > PositionTolerance)
            {
                return false;
            }
            if (Float.Abs(diff.y) > PositionTolerance)
            {
                return false;
            }

            float da = a.Angle % 360f, db = b.Angle % 360f;

            if (da < 0)
            {
                da = 360f - (Float.Abs(da));
            }
            if (db < 0)
            {
                db = 360f - (Float.Abs(db));
            }

            da = Float.Abs(da - db);

            if (da == 180)
            {
                if (a.Scale.x == b.Scale.x * -1 && a.Scale.y == b.Scale.y * -1)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else if (da > RotationTolerance)
            {
                return false;
            }

            diff = a.Scale - b.Scale;
            if (Float.Abs(diff.x) > ScaleTolerance)
            {
                return false;
            }
            if (Float.Abs(diff.y) > ScaleTolerance)
            {
                return false;
            }

            return true;
        }

        #region Masking
        
        public void GetMaskIntersectedSymmetries(Spline2[] mask, out int[] symIDs, out int[] maskIntersectionIDs)
        {
            GetMaskIntersectedSymmetries(mask, GetMaskedSymmetries(mask), out symIDs, out maskIntersectionIDs);
        }

        public void GetMaskIntersectedSymmetries(Spline2[] mask, int[] maskedIDs, out int[] symIDs, out int[] maskIntersectionIDs)
        {
            List<int> masked = new List<int>(maskedIDs);
            List<int> intersectIDs = new List<int>(masked.Count);

            //Starting with our masked symmetriess, we go through each and check it against each spline in mask. If it's intersected
            //by a mask, we keep it and update its intersectID accordingly. If it DOESN'T, we remove its entry from both lists.
            for (int i = masked.Count - 1; i >= 0; i--)
            {
                bool intersected = false;
                Float2[] symVerts = GetSymmetryVertices(masked[i]);

                for (int j = 0; j < mask.Length; j++)
                {
                    for (int v = 0; v < symVerts.Length - 1; v++)
                    {
                        if (mask[j].LineIntersects(symVerts[v], symVerts[v + 1]))
                        {
                            intersectIDs[i] = j;
                            intersected = true;
                            break;
                        }
                    }
                    if (intersected) { break; }
                }
                if (!intersected)
                {
                    masked.RemoveAt(i);
                    intersectIDs.RemoveAt(i);
                }
            }

            symIDs = masked.ToArray();
            maskIntersectionIDs = intersectIDs.ToArray();
        }

        public int[] GetMaskedSymmetries(Spline2[] mask, bool thoroughCheck = false)
        {
            List<int> ids = new List<int>();
            for (int i = 0; i < symmetries.Length; i++)
            {
                if (mask.ContainsPoint(symmetries[i].Position) >= 0)
                {
                    ids.Add(i);
                }
                else if (thoroughCheck)
                {
                    int[] verts = symmetries[i].Vertices;
                    for (int j = verts.Length - 1; j >= 0; j--)
                    {
                        if (mask.ContainsPoint(Vertices[verts[j]]) >= 0)
                        {
                            ids.Add(i);
                            break;
                        }
                    }
                }
            }
            return ids.ToArray();
        }

        public int[] GetUnmaskedSymmetries(Spline2[] mask, bool thoroughCheck = false)
        {
            List<int> ids = new List<int>();

            for (int i = 0; i < symmetries.Length; i++)
            {
                bool addThis = true;
                if (mask.ContainsPoint(symmetries[i].Position) >= 0)
                {
                    addThis = false;
                }
                else if (thoroughCheck)
                {
                    int[] verts = symmetries[i].Vertices;
                    for (int j = verts.Length - 1; j >= 0; j--)
                    {
                        if (mask.ContainsPoint(Vertices[verts[j]]) >= 0)
                        {
                            addThis = false;
                            break;
                        }
                    }
                }
                if (addThis)
                {
                    ids.Add(i);
                }
            }
            return ids.ToArray();
        }

        public bool[] GetMaskStates(Spline2[] mask, bool thoroughCheck = false)
        {
            bool[] masked = new bool[symmetries.Length];
            for (int i = 0; i < symmetries.Length; i++)
            {
                if (mask.ContainsPoint(symmetries[i].Position) >= 0)
                {
                    masked[i] = true;
                }
                else if (thoroughCheck)
                {
                    int[] verts = symmetries[i].Vertices;
                    for (int j = verts.Length - 1; j >= 0; j--)
                    {
                        if (mask.ContainsPoint(Vertices[verts[j]]) >= 0)
                        {
                            masked[i] = true;
                            break;
                        }
                    }
                }
            }
            return masked;
        }

        #endregion

        public virtual void Offset(Float2 offset)
        {
            for (int i = vertices.Length - 1; i >= 0; i--)
            {
                vertices[i] += offset;
            }
            for (int i = symmetries.Length - 1; i >= 0; i--)
            {
                symmetries[i].Position = symmetries[i].Position + offset;
            }
        }

        public virtual void Scale(Float2 scalar)
        {
            for (int i = vertices.Length - 1; i >= 0; i--)
            {
                vertices[i] *= scalar;
            }
            for (int i = symmetries.Length - 1; i >= 0; i--)
            {
                symmetries[i].Position = symmetries[i].Position * scalar;
            }
        }

        #region Affine Transforms

        public void Translate(ref TSymmetry symmetry, Float2 translation, bool localTranslation = false)
        {
            if (!localTranslation)
            {
                symmetry.Position += translation;
                return;
            }

            Matrix3x3 translationMatrix = Matrix3x3.TranslationMatrix(translation);

            Matrix3x3 scaleMatrix = Matrix3x3.ScaleMatrix(symmetry.Scale);

            Matrix3x3 rotationMatrix = Matrix3x3.RotationMatrix(symmetry.Angle);

            symmetry.Position += Float2.zero * (scaleMatrix * rotationMatrix * translationMatrix);
        }

        public void RotateAround(ref TSymmetry symmetry, Float2 pivot, float angle, bool localPivot = false)
        {
            if (localPivot) { pivot *= Matrix3x3.GlobalToLocalMatrix(symmetry.Angle, symmetry.Scale, symmetry.Position); }

            Matrix3x3 trans = Matrix3x3.RotationAroundPointMatrix(pivot, symmetry.Angle);

            symmetry.Position *= trans;

            symmetry.Angle += angle;

        }

        public void GlideReflect(ref TSymmetry symmetry, Float2 reflectLineStart, Float2 reflectLineEnd, bool isLocalReflection = true)
        {
            if (isLocalReflection) 
            {
                Matrix3x3 globalToLocal =  Matrix3x3.GlobalToLocalMatrix(symmetry.Angle, symmetry.Scale, symmetry.Position);
                reflectLineStart *= globalToLocal;
                reflectLineEnd *= globalToLocal;
            }

            Matrix3x3 reflectionMatrix = Matrix3x3.ReflectionMatrix(reflectLineStart, reflectLineEnd);

            Float2 v1 = new Float2(symmetry.Position.x - reflectLineStart.x, symmetry.Position.x - reflectLineStart.y);  

            var gr = (v1 * reflectionMatrix) + reflectLineStart;
            symmetry.Position = gr;
            symmetry.Scale = new Float2(reflectionMatrix.M11, reflectionMatrix.M22);

        }

        public bool Equals(TGroup other)
        {
            return other == this;
        }

        #endregion

        #endregion


    }

}
