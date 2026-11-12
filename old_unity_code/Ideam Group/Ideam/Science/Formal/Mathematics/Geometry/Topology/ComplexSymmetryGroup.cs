using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    public enum SymmetryGroupType
    {
        None, P1, P2, PM, PG, CM,
        P2MM, P2MG, P2GG, C2MM,
        P4, P4MM, P4MG, P3, P3M1,
        P31M, P6, P6MM
    }

    [Serializable]
    public class ComplexSymmetryGroup : SymmetryGroup<ComplexSymmetryGroup, IsoSymmetry>
    {
        #region Sub Classes
        public class SymGroup
        {
            public ComplexSymmetryGroup group;
            public int priority;
        }
        #endregion

        #region Variables

        //[SerializeField]
        public int[] hexCenters = new int[0], hexEdges = new int[0];

        public virtual int[] HexCenterIDS { get { return hexCenters; } }

        public virtual int[] HexEdgeIDs
        {
            get { return hexEdges; }
        }

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

        public virtual bool HexContainsPoint(Float2 p, ref int hexID)
        {
            int cellID = -1;
            if (ContainsPoint(p, ref cellID))
            {
                //I know the cell that contains it. First, I want to find which of its points is a hex center.
                int[] cellVerts = symmetries[cellID].Vertices;
                for (int i = cellVerts.Length - 1; i >= 0; i--)
                {
                    for (int j = hexCenters.Length - 1; j >= 0; j--)
                    {
                        if (cellVerts[i] == hexCenters[j])
                        {
                            hexID = j;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        #endregion

        #region Properties

        public override float PositionTolerance { get; set; }
        public override float RotationTolerance { get; set; }
        public override float ScaleTolerance { get; set; }

        public Float2[] primitiveVertices = new Float2[0];
        public override Float2[] PrimitiveVertices 
        {
            get 
            {
                return primitiveVertices;
            } 
        }
        #endregion

        #region Methods

        #region Constructors

        public static ComplexSymmetryGroup Wallpaper(SymmetryGroupType wallpaperType, Spline2[] region, Float2 start, Action<IEnumerator> buildAction)
        {
            ComplexSymmetryGroup wallpaper = new ComplexSymmetryGroup();

            switch (wallpaperType)
            {
                case SymmetryGroupType.P1: 
                    wallpaper = Convert<P1, Symmetry4> (new P1(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.PG: 
                    wallpaper = Convert<PG, Symmetry6> (new PG(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.PM: 
                    wallpaper = Convert<PM, Symmetry4> (new PM(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P2: 
                    wallpaper = Convert<P2, Symmetry4> (new P2(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P2MM: 
                    wallpaper = Convert<P2MM, Symmetry4> (new P2MM(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P2MG: 
                    wallpaper = Convert<P2MG, Symmetry4> (new P2MG(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P2GG: 
                    wallpaper = Convert<P2GG, Symmetry4> (new P2GG(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P3: 
                    wallpaper = Convert<P3, Symmetry4> (new P3(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P31M:
                    wallpaper = Convert<P31M, Symmetry3> (new P31M(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P3M1: 
                    wallpaper = Convert<P3M1, Symmetry3> (new P3M1(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P4: 
                    wallpaper = Convert<P4, Symmetry4> (new P4(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P4MG: 
                    wallpaper = Convert<P4MG, Symmetry3> (new P4MG(region, start, buildAction));
                    break;

                case SymmetryGroupType.P4MM: 
                    wallpaper = Convert<P4MM, Symmetry3> (new P4MM(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P6: 
                    wallpaper = Convert<P6, Symmetry3> (new P6(region, start, buildAction)); 
                    break;

                case SymmetryGroupType.P6MM: 
                    wallpaper = Convert<P6MM, Symmetry3> (new P6MM(region, start, buildAction)); 
                    break;
            }

            return wallpaper;
        }

        public static IEnumerator WallpaperProcess(SymmetryGroupType wallpaperType, Spline2[] region, Float2 start, Action<ComplexSymmetryGroup> groupCompleteAction)
        {
            ComplexSymmetryGroup newGroup = new ComplexSymmetryGroup();
            IEnumerator initialGridBuild;
            switch (wallpaperType)
            {
                case SymmetryGroupType.None: yield return null; groupCompleteAction?.Invoke(newGroup); yield break;

                case SymmetryGroupType.P1:

                    P1 p1 = new P1();

                    initialGridBuild = p1.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P1, Symmetry4>(p1);

                    break;

                case SymmetryGroupType.P2:

                    P2 p2 = new P2();

                    initialGridBuild = p2.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P2, Symmetry4>(p2);

                    break;

                case SymmetryGroupType.PM:

                    PM pM = new PM();

                    initialGridBuild = pM.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<PM, Symmetry4>(pM);

                    break;


                case SymmetryGroupType.PG:

                    PG pG = new PG();

                    initialGridBuild = pG.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<PG, Symmetry6>(pG);

                    break;

                case SymmetryGroupType.CM:

                    CM cM = new CM();

                    initialGridBuild = cM.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<CM, Symmetry3>(cM);

                    break;

                case SymmetryGroupType.P2MM:

                    P2MM p2MM = new P2MM();

                    initialGridBuild = p2MM.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P2MM, Symmetry4>(p2MM);

                    break;

                case SymmetryGroupType.P2MG:

                    P2MG p2MG = new P2MG();

                    initialGridBuild = p2MG.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P2MG, Symmetry4>(p2MG);

                    break;

                case SymmetryGroupType.P2GG:

                    P2GG p2GG = new P2GG();

                    initialGridBuild = p2GG.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P2GG, Symmetry4>(p2GG);

                    break;

                case SymmetryGroupType.C2MM:

                    C2MM c2MM = new C2MM();

                    initialGridBuild = c2MM.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<C2MM, Symmetry3>(c2MM);

                    break;

                case SymmetryGroupType.P4:

                    P4 p4 = new P4();

                    initialGridBuild = p4.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P4, Symmetry4>(p4);

                    break;

                case SymmetryGroupType.P4MM:

                    P4MM p4MM = new P4MM();

                    initialGridBuild = p4MM.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P4MM, Symmetry3>(p4MM);

                    break;

                case SymmetryGroupType.P4MG:

                    P4MG p4MG = new P4MG();

                    initialGridBuild = p4MG.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P4MG, Symmetry3>(p4MG);

                    break;

                case SymmetryGroupType.P3:

                    P3 p3 = new P3();

                    initialGridBuild = p3.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P3, Symmetry4>(p3);

                    break;

                case SymmetryGroupType.P3M1:

                    P3M1 p3M1 = new P3M1();

                    initialGridBuild = p3M1.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P3M1, Symmetry3>(p3M1);

                    break;

                case SymmetryGroupType.P31M:

                    P31M p31M = new P31M();

                    initialGridBuild = p31M.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P31M, Symmetry3>(p31M);

                    break;

                case SymmetryGroupType.P6:

                    P6 p6 = new P6();

                    initialGridBuild = p6.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P6, Symmetry3>(p6);

                    break;

                case SymmetryGroupType.P6MM:

                    P6MM p6MM = new P6MM();

                    initialGridBuild = p6MM.BuildGroup(region, start, true);
                    while (initialGridBuild.MoveNext()) { yield return null; }

                    newGroup = Convert<P6MM, Symmetry3>(p6MM);

                    break;
            }

            yield return null;

            groupCompleteAction?.Invoke(newGroup);

        }

        #endregion

        #endregion

        public override void Setup()
        {

        }

        public static ComplexSymmetryGroup Convert<TGroup, TSymmetry>(TGroup group) 
            where TGroup : SymmetryGroup<TGroup, TSymmetry>
            where TSymmetry : ISymmetry
        {
            ComplexSymmetryGroup newGroup = new ComplexSymmetryGroup();
            newGroup.symmetries = IsoSymmetry.ConvertSymmetries(group.Symmetries);
            newGroup.symmetryTemplate = new IsoSymmetry(group.symmetryTemplate);
            newGroup.vertices = group.Vertices;
            newGroup.edges = group.Edges;
            newGroup.primitiveVertices = group.PrimitiveVertices;

            newGroup.borderSymmetries = group.BorderSymmetries;
            newGroup.borderEdges = group.BorderEdges;
            newGroup.borderVertices = group.BorderVertices;
            newGroup.updateBorders = false;

            return newGroup;
        }


        public IEnumerator MergeGroup(ComplexSymmetryGroup group)
        {
            //First, an important check: If this is a fresh ComplexSymmetryGroup, let's just copy the supplied group, wholesale.
            if (symmetries.Length == 0)
            {
                symmetries = group.Symmetries;
                vertices = group.Vertices;
                edges = group.Edges;

                borderSymmetries = group.BorderSymmetries;
                borderEdges = group.BorderEdges;
                borderVertices = group.BorderVertices;
                updateBorders = false;
                yield break;
            }

            // To start, we'll be adding some possibly strange assortments of cells, verts, and edges, so we're going to 
            // merge the incoming group into lists for ease of use and sanity.
            List<IsoSymmetry> newSymmetries = new List<IsoSymmetry>(symmetries);

            List<Float2> newVerts = new List<Float2>(vertices);

            List<SymmetryEdge> newEdges = new List<SymmetryEdge>(edges);

            //Prolly don't need these.
            List<int> newBorderSymmetries = new List<int>(borderSymmetries),
                newBorderEdges = new List<int>(borderEdges),
                newBorderPoints = new List<int>(borderVertices);

            // We'll use this mask to determine whether or not the incoming group overlaps with our existing group.
            Spline2[] mask = Outline;

            // Each entry in this list is the ID number of a cell that is being at least partly overlapped by
            // our present group. The incoming group is always the group that gets pruned, in the event of overlaps.
            int[] maskedIDs = group.GetMaskedSymmetries(mask, true);

            // These are the parts we'll be merging into our present group:
            IsoSymmetry[] groupSymmetries = group.Symmetries;
            Float2[] groupVerts = group.Vertices;
            SymmetryEdge[] groupEdges = group.Edges;

            // And we'll use these to assign new reference ids to incoming cells, verts, and edges.
            int[] groupSymmetryIDs = new int[groupSymmetries.Length],
                  groupVertIDs = new int[groupVerts.Length],
                  groupEdgeIDs = new int[groupEdges.Length];

            // If we have any masked ids, it means our areas overlap. 
            // We'll need to prune some incoming cells and build bridge cells.
            if (maskedIDs.Length > 0)
            {
                // Later, we'll need to know which cells are NOT masked, but ARE touching a cell that gets 
                // intersected by the masks edges. 
                bool[] isMasked = new bool[groupSymmetries.Length];
                for (int i = maskedIDs.Length - 1; i >= 0; i--)
                {
                    isMasked[maskedIDs[i]] = true;
                }

                // Next, we want to know which masked cells were intersected by the mask. These border cells
                // will help us identify which Dominant edges we're going to build from, later on.
                int[] intersectedIDs, maskIDs;

                group.GetMaskIntersectedSymmetries(mask, maskedIDs, out intersectedIDs, out maskIDs);

                // These will identify which edges of the current group are going to be bridged to which edges of the incoming
                // group.
                List<int> currentBridgeEdges = new List<int>(),
                          currentBEIDs = new List<int>(),
                          currentBridgeIDs = new List<int>(),
                          groupBridgeEdges = new List<int>(),
                          groupBEIDs = new List<int>(),
                          groupBridgeIDs = new List<int>();

                // These will hold references to the neighboring cells of the bridge cells we're going to build, as well
                // as holding references to the newly created bridge cells, themselves.
                List<IsoSymmetry> currentBridge = new List<IsoSymmetry>(),
                               groupBridge = new List<IsoSymmetry>(),
                               bridgeSymmetries = new List<IsoSymmetry>();

                // This holds the newly built edges for each of our bridge cells.
                List<SymmetryEdge> bridgeEdges = new List<SymmetryEdge>();

                // At last, we begin by breaking down which current cells will be bridged to which incoming cells.
                // Only border cells are considered as possible bridge sites, since the goal is to preserve the 
                // present group.
                for (int i = newBorderSymmetries.Count - 1; i >= 0; i--)
                {
                    // This cell is considered the dominant cell, since it must be preserved. 
                    IsoSymmetry dominantSymmetry = newSymmetries[newBorderSymmetries[i]];

                    // We gather its vertices.
                    int[] pid = dominantSymmetry.Vertices;
                    Float2[] pts = new Float2[pid.Length];
                    for (int j = pid.Length - 1; j >= 0; j--)
                    {
                        pts[j] = newVerts[pid[j]];
                    }

                    // Now we go through all of the cells on our incoming group that were intersected by one of the masks
                    // borders. These cells are ineligible - they'll be removed to clear the way for our bridge cells.
                    // It's cells that share a border with them and are NOT masked that we're after: Symmetries and edges that are 
                    // entirely clear of the mask, and that have newly exposed edges that can be bridge sites.
                    for (int j = intersectedIDs.Length - 1; j >= 0; j--)
                    {
                        // As said above, this cell is ineligible as a bridge site.
                        IsoSymmetry borderSymmetry = new IsoSymmetry(groupSymmetries[intersectedIDs[j]]);
                        int[] bid = borderSymmetry.Vertices, bed = borderSymmetry.Edges;

                        // So we go through its edges. 
                        for (int k = 0; k < bed.Length; k++)
                        {
                            SymmetryEdge cEdge = groupEdges[bed[k]];

                            // We need to know if the edge believes it was cell A or B that was intersected. 
                            // Depending on that, the OTHER cell (should it exist) is the cell we want to examine.
                            if (cEdge.nodeA == intersectedIDs[j])
                            {
                                // If cell b DOES exist and IS NOT masked, it's the cell we're looking for!
                                if (cEdge.nodeB >= 0 && !isMasked[groupEdges[bed[k]].nodeB])
                                {
                                    // We need a reference to that cell, and the id slot IT'S using to reference 
                                    // the edge that got us here in the first place.
                                    IsoSymmetry cellB = new IsoSymmetry(groupSymmetries[cEdge.nodeB]);

                                    int[] cbEdges = cellB.Edges;
                                    for (int l = cbEdges.Length - 1; l >= 0; l--)
                                    {
                                        // If this is the edge we used to find cell B, that's the reference we want.
                                        if (groupEdges[cbEdges[l]] == cEdge)
                                        {
                                            // References found, we add them to our incoming group lists.
                                            groupBridge.Add(cellB);
                                            groupBEIDs.Add(bed[k]);
                                            groupBridgeIDs.Add(cEdge.nodeB);
                                            groupBridgeEdges.Add(l);
                                        }
                                    }
                                }
                            }
                            else if (cEdge.nodeB == intersectedIDs[j])
                            {
                                // As above, so below. The only difference is that, here we're considering cell A instead of B.
                                if (cEdge.nodeA >= 0 && !isMasked[groupEdges[bed[k]].nodeA])
                                {
                                    IsoSymmetry cellA = new IsoSymmetry(groupSymmetries[cEdge.nodeA]);
                                    int[] cbEdges = cellA.Edges;
                                    for (int l = cbEdges.Length - 1; l >= 0; l--)
                                    {
                                        if (groupEdges[cbEdges[l]] == cEdge)
                                        {
                                            groupBridge.Add(cellA);
                                            groupBEIDs.Add(bed[k]);
                                            groupBridgeIDs.Add(cEdge.nodeA);
                                            groupBridgeEdges.Add(l);
                                        }
                                    }
                                }
                            }
                        }
                        yield return null;

                        // Now we go through each of the border cells edges. We're going to compare them to each
                        // present edges to find which edge of our existing cells were crossed. Crossed edges require
                        // bridges to be built.
                        for (int k = 0; k < bid.Length - 1; k++)
                        {
                            for (int l = 0; l < pts.Length - 1; l++)
                            {
                                // Line Intersection returns the point of intersection, should one exist. This
                                // Float2 holds that info, even though we do not presently need it. 
                                // Fancier stitching algorithms could very likely want this info, however, so here it stays.
                                Float2 cPoint = Float2.zero;

                                // If the two edges we're presently looking at - one on a current cell, one on an incoming cell -
                                // intersect one another, we add the current cell and its current edge id to our current lists.
                                if (Float2.LineIntersection(pts[l], pts[l + 1], groupVerts[bid[k]], groupVerts[bid[k + 1]], ref cPoint))
                                {
                                    currentBridge.Add(dominantSymmetry);
                                    currentBEIDs.Add(dominantSymmetry.Edges[l]);
                                    currentBridgeIDs.Add(newBorderSymmetries[i]);
                                    currentBridgeEdges.Add(l);
                                }
                            }
                        }
                        yield return null;
                    }
                }

                // From among our bridge edges from both current and incoming cells, we go through and match as many points 
                // as we can between the two. With these new edges, we create bridge cells out of quads.
                // Whichever list is smaller dictates the number of bridge cells we can create.
                int lowest = currentBridgeEdges.Count;
                if (groupBridgeEdges.Count < lowest)
                {
                    lowest = groupBridgeEdges.Count;
                }

                for (int i = 0; i < lowest; i++)
                {
                    // We take a current edge.
                    // We compare both of its points with each point from the new edges. We find the one with the lowest distances
                    // Then we make a cell with those four points and with the two old edges and the two new ones bridging them.

                    IsoSymmetry dominantSymmetry = currentBridge[i];
                    int dominantEdgeID = dominantSymmetry.Edges[currentBridgeEdges[i]];
                    SymmetryEdge dominantEdge = newEdges[dominantEdgeID];

                    // The points of this bridge edge
                    Float2 dA = newVerts[dominantEdge.vertexA],
                        dB = newVerts[dominantEdge.vertexB];

                    // We start looking through the others by looking for the lowest combination of two point distances we can
                    // find.
                    float dist1 = float.MaxValue, dist2 = float.MaxValue;

                    // And we store the ID number for that pair, as well as the order to bridge those pairs, in these.
                    int bestDistance = -1; int bestMatch = -1;
                    Float2 bestVertexA = Float2.one, bestVertexB = Float2.one;
                    IsoSymmetry groupSymmetry;
                    SymmetryEdge groupEdge;
                    int groupEdgeID;
                    bool[] usedGroup = new bool[lowest];
                    // Next we go through all of the incoming bridge edges and start comparing point distance pairs.
                    for (int j = lowest - 1; j >= 0; j--)
                    {
                        if (usedGroup[j]) { continue; }

                        groupSymmetry = groupBridge[j];
                        groupEdgeID = groupSymmetry.Edges[groupBridgeEdges[j]];
                        groupEdge = groupEdges[groupEdgeID];

                        //The points of this incoming edge.
                        Float2 gA = groupVerts[groupEdge.vertexA],
                            gB = newVerts[groupEdge.vertexB];

                        // We calculate four distances based on each ordered pair of point possibilities.
                        float distAA = Float2.Distance(dA, gA),
                              distAB = Float2.Distance(dA, gB),
                              distBA = Float2.Distance(dB, gA),
                              distBB = Float2.Distance(dB, gB);

                        // And, should any of the following four possibilities beat out both distances, it
                        // becomes our new best bridge site.
                        // NOTE! Originally, bestMatch could be 0-3, hence the four possibilities. I think I only need two, however.
                        if ((distAA < dist1 && distBB < dist2))
                        {
                            dist1 = distAA; dist2 = distBB;
                            bestDistance = j;
                            bestMatch = 0;
                            bestVertexA = gA;
                            bestVertexB = gB;
                        }
                        else if ((distAA < dist2 && distBB < dist1))
                        {
                            dist1 = distBB; dist2 = distAA;
                            bestDistance = j;
                            bestMatch = 0;
                            bestVertexA = gA;
                            bestVertexB = gB;
                        }
                        else if ((distAB < dist1 && distBA < dist2))
                        {
                            dist1 = distAB; dist2 = distBA;
                            bestDistance = j;
                            bestMatch = 1;
                            bestVertexA = gA;
                            bestVertexB = gB;
                        }
                        else if ((distAB < dist2 && distBA < dist1))
                        {
                            dist1 = distBA; dist2 = distAB;
                            bestDistance = j;
                            bestMatch = 1;
                            bestVertexA = gA;
                            bestVertexB = gB;
                        }
                    }

                    //Okay, now we know which pair of points best matches the pair we're currently looking at.
                    //I collect both of those edges, and create two new ones, based on bestMatch.

                    groupSymmetry = groupBridge[bestDistance];
                    groupEdgeID = groupSymmetry.Edges[groupBridgeEdges[bestDistance]];
                    groupEdge = groupEdges[groupEdgeID];
                    usedGroup[bestDistance] = true;
                    //I then create a cell with those four points and edges.
                    //I must also remember to add it in this order:
                    //Dominant Edge, Group Edge, New Edge 1, New Edge 2.
                    //The cells references will, at present, refer to the original dominant edge id, the original group edge
                    //id, then the ids for bridgeEdges;
                    SymmetryEdge bridgeEdge1 = new SymmetryEdge(), bridgeEdge2 = new SymmetryEdge();
                    IsoSymmetry bridgeSymmetry = new IsoSymmetry();
                    bridgeSymmetry.Position = (dA + dB + bestVertexB + bestVertexA) * 0.25f;
                    switch (bestMatch)
                    {
                        case 0:
                            // We'll march from dA -> dB -> gB -> gA
                            // Important to note that we may well have to reverse this to get it in clockwise order.
                            bridgeSymmetry.Vertices = new int[] { dominantEdge.vertexA, dominantEdge.vertexB, groupEdge.vertexB, groupEdge.vertexA };

                            bridgeEdge1 = new SymmetryEdge(dominantEdge.vertexA, groupEdge.vertexA);

                            bridgeEdge2 = new SymmetryEdge(dominantEdge.vertexB, groupEdge.vertexB);
                            break;
                        case 1:
                            // We'll march from dA -> dB -> gA -> gB
                            bridgeSymmetry.Vertices = new int[] { dominantEdge.vertexA, dominantEdge.vertexB, groupEdge.vertexA, groupEdge.vertexB };

                            bridgeEdge1 = new SymmetryEdge(dominantEdge.vertexA, groupEdge.vertexB);

                            bridgeEdge2 = new SymmetryEdge(dominantEdge.vertexB, groupEdge.vertexA);
                            break;

                    }
                    bool found1 = false, found2 = false;
                    int id1 = bridgeEdges.Count, id2 = bridgeEdges.Count + 1;

                    bridgeEdge1.nodeA = bridgeEdge2.nodeA = bridgeSymmetries.Count;
                    for (int j = bridgeEdges.Count - 1; j >= 0; j--)
                    {
                        if (bridgeEdges[j] == bridgeEdge1)
                        {
                            found1 = true;
                            bridgeEdge1.nodeB = bridgeEdges[j].nodeA;
                            bridgeEdges[j] = bridgeEdge1;
                            id1 = j;
                        }
                        if (bridgeEdges[j] == bridgeEdge2)
                        {
                            found2 = true;
                            bridgeEdge2.nodeB = bridgeEdges[j].nodeA;
                            bridgeEdges[j] = bridgeEdge2;
                            id2 = j;
                        }
                    }
                    if (!found1) { bridgeEdges.Add(bridgeEdge1); }
                    if (!found2) { bridgeEdges.Add(bridgeEdge2); }

                    bridgeSymmetry.Neighbors = new int[] { currentBridgeIDs[i], bridgeEdge1.nodeB, groupBridgeIDs[bestDistance], bridgeEdge2.nodeB };
                    bridgeSymmetry.Edges = new int[] { dominantSymmetry.Edges[currentBridgeEdges[i]], id1, groupSymmetry.Edges[groupBridgeEdges[bestDistance]], id2 };
                    bridgeSymmetry.Generation = dominantSymmetry.Generation + 1;
                    bridgeSymmetries.Add(bridgeSymmetry);

                    int bcid = bridgeSymmetries.Count - 1;

                    int[] idUpdates = dominantSymmetry.Neighbors;

                    idUpdates[currentBridgeEdges[i]] = bcid;
                    dominantSymmetry.Neighbors = idUpdates;

                    dominantEdge.nodeB = bcid;
                    newEdges[dominantEdgeID] = dominantEdge;

                    idUpdates = groupSymmetry.Neighbors;
                    idUpdates[groupBridgeEdges[bestDistance]] = bcid;
                    groupSymmetry.Neighbors = idUpdates;

                    groupEdge.nodeB = bcid;
                    groupEdges[groupEdgeID] = groupEdge;

                    yield return null;
                }

                //Okay. Time to update my references.
                //First, a first pass on all of our group cell ids, and a cumulative update on our vert IDS.
                int offset = newSymmetries.Count, removed = 0;
                for (int i = 0; i < groupSymmetries.Length; i++)
                {
                    if (isMasked[i])
                    {
                        removed++; groupSymmetryIDs[i] = -1;
                    }
                    else
                    {
                        groupSymmetryIDs[i] = offset + i - removed;
                        int[] gcverts = groupSymmetries[i].Vertices;
                        for (int j = gcverts.Length - 1; j >= 0; j--)
                        {
                            groupVertIDs[gcverts[j]]++;
                        }
                    }
                }

                // Now we update all of our vert ids.
                offset = newVerts.Count;
                removed = 0;
                for (int i = 0; i < groupVerts.Length; i++)
                {
                    if (groupVertIDs[i] > 0)
                    {
                        groupVertIDs[i] = offset + i - removed;
                        newVerts.Add(groupVerts[i]);
                    }
                    else
                    {
                        groupVertIDs[i] = -1;
                        removed++;
                    }
                }

                // Next, we update all of our edge ids, as well as their cell references.
                offset = newEdges.Count;
                removed = 0;
                for (int i = 0; i < groupEdges.Length; i++)
                {
                    SymmetryEdge groupEdge = groupEdges[i];
                    if (groupEdge.nodeA >= 0) { groupEdge.nodeA = groupSymmetryIDs[groupEdge.nodeA]; }
                    if (groupEdge.nodeB >= 0) { groupEdge.nodeB = groupSymmetryIDs[groupEdge.nodeB]; }

                    groupEdge.vertexA = groupVertIDs[groupEdge.vertexA];
                    groupEdge.vertexB = groupVertIDs[groupEdge.vertexB];

                    if (groupEdge.nodeA < 0 && groupEdge.nodeB < 0)
                    {
                        groupEdgeIDs[i] = -1;
                        removed++;
                    }
                    else
                    {
                        newEdges.Add(groupEdge);

                        groupEdges[i] = groupEdge;

                        groupEdgeIDs[i] = offset + i - removed;
                    }
                }

                yield return null;

                // Finally, we pass one last time over the cells themselves, to update their edge, neighbor, neighbor, and vert ids.
                for (int i = 0; i < groupSymmetries.Length; i++)
                {
                    if (groupSymmetryIDs[i] < 0) { continue; }

                    IsoSymmetry groupSymmetry = groupSymmetries[i];

                    int[] cellData = groupSymmetry.Vertices;
                    for (int j = cellData.Length - 1; j >= 0; j--)
                    {
                        cellData[j] = groupVertIDs[cellData[j]];
                    }
                    groupSymmetry.Vertices = cellData;

                    cellData = groupSymmetry.Edges;
                    for (int j = cellData.Length - 1; j >= 0; j--)
                    {
                        if (cellData[j] < 0) { continue; }
                        cellData[j] = groupEdgeIDs[cellData[j]];
                    }
                    groupSymmetry.Edges = cellData;

                    cellData = groupSymmetry.Neighbors;
                    for (int j = cellData.Length - 1; j >= 0; j--)
                    {
                        if (cellData[j] < 0) { continue; }
                        cellData[j] = groupSymmetryIDs[cellData[j]];
                    }
                    groupSymmetry.Neighbors = cellData;

                    cellData = groupSymmetry.Neighbors;
                    for (int j = cellData.Length - 1; j >= 0; j--)
                    {
                        if (cellData[j] < 0) { continue; }
                        cellData[j] = groupSymmetryIDs[cellData[j]];
                    }
                    groupSymmetry.Neighbors = cellData;

                    newSymmetries.Add(groupSymmetry);
                    yield return null;
                }


                // Now, at last, it's time to add our bridge cells and bridge edges.
                // We start with the bridge cells.
                offset = newSymmetries.Count;
                int eOffset = newEdges.Count;
                for (int i = 0; i < bridgeSymmetries.Count; i++)
                {
                    IsoSymmetry bridgeSymmetry = groupSymmetries[i];

                    int[] cellData = bridgeSymmetry.Vertices;
                    //We leave 0, and 1 alone as those ids haven't been updated.
                    cellData[2] = groupVertIDs[cellData[2]];
                    cellData[3] = groupVertIDs[cellData[3]];

                    bridgeSymmetry.Vertices = cellData;

                    cellData = bridgeSymmetry.Edges;
                    //We leave 0 alone. 1 is from our group lists. 2 and 3 are bridge edges.
                    cellData[1] = groupEdgeIDs[cellData[1]];
                    cellData[2] += eOffset;
                    cellData[3] += eOffset;
                    bridgeSymmetry.Edges = cellData;

                    cellData = bridgeSymmetry.Neighbors;
                    //0 should be old, 1 should be bridge, 2 should be group, 3 should be bridge.
                    bridgeSymmetry.Neighbors = cellData;
                    cellData[1] += offset;
                    cellData[2] = groupSymmetryIDs[cellData[2]];
                    cellData[3] += offset;

                    newSymmetries.Add(bridgeSymmetry);
                }

                // Next, the cell and vert ids of each bridge edge.
                for (int i = 0; i < bridgeEdges.Count; i++)
                {
                    SymmetryEdge bridgeEdge = bridgeEdges[i];
                    bridgeEdge.nodeA += offset;
                    bridgeEdge.nodeB += offset;
                    //Vertex A is part of the current group, and its ids have not changed.
                    bridgeEdge.vertexB = groupVertIDs[bridgeEdge.vertexB];

                    newEdges.Add(bridgeEdge);
                }

                //We also need to update:
                // -The old border cells that are now neighbors of the bridge cells.
                for (int i = 0; i < currentBridge.Count; i++)
                {

                    IsoSymmetry dominantSymmetry = currentBridge[i];
                    int dominantEdgeID = dominantSymmetry.Edges[currentBridgeEdges[i]];
                    SymmetryEdge dominantEdge = newEdges[dominantEdgeID];

                    int[] idUpdates = dominantSymmetry.Neighbors;
                    idUpdates[currentBridgeEdges[i]] += offset;
                    dominantSymmetry.Neighbors = idUpdates;

                    dominantEdge.nodeB += offset;
                    newEdges[dominantEdgeID] = dominantEdge;

                }
                // -And, the group border cells, which are also neighbors to the bridge cells.
                for (int i = 0; i < groupBridge.Count; i++)
                {

                    IsoSymmetry groupSymmetry = groupBridge[i];
                    int groupEdgeID = groupSymmetry.Edges[groupBridgeEdges[i]];
                    SymmetryEdge groupEdge = newEdges[groupEdgeID];

                    int[] idUpdates = groupSymmetry.Neighbors;
                    idUpdates[groupBridgeEdges[i]] += offset;
                    groupSymmetry.Neighbors = idUpdates;

                    groupEdge.nodeB += offset;
                    newEdges[groupEdgeID] = groupEdge;

                }

                //And, Fuuuuuusiioooooon-HA!
            }
            else
            {
                //We start by updating our cell ids.
                int offset = newSymmetries.Count;
                for (int i = 0; i < groupSymmetries.Length; i++)
                {
                    groupSymmetryIDs[i] = offset + i;
                }

                // Now we update all of our vert ids.
                offset = newVerts.Count;
                for (int i = 0; i < groupVerts.Length; i++)
                {
                    groupVertIDs[i] = offset + i;
                }

                // Next, we update all of our edge ids, as well as their cell references.
                offset = newEdges.Count;
                for (int i = 0; i < groupEdges.Length; i++)
                {
                    SymmetryEdge groupEdge = groupEdges[i];
                    if (groupEdge.nodeA >= 0) { groupEdge.nodeA = groupSymmetryIDs[groupEdge.nodeA]; }
                    if (groupEdge.nodeB >= 0) { groupEdge.nodeB = groupSymmetryIDs[groupEdge.nodeB]; }

                    groupEdge.vertexA = groupVertIDs[groupEdge.vertexA];
                    groupEdge.vertexB = groupVertIDs[groupEdge.vertexB];

                    newEdges.Add(groupEdge);

                    groupEdges[i] = groupEdge;

                    groupEdgeIDs[i] = offset + i;

                }
                yield return null;
                // Finally, we pass one last time over the cells themselves, to update their edge, neighbor, neighbor, and vert ids.
                for (int i = 0; i < groupSymmetries.Length; i++)
                {
                    if (groupSymmetryIDs[i] < 0) { continue; }

                    IsoSymmetry groupSymmetry = groupSymmetries[i];

                    int[] cellData = groupSymmetry.Vertices;
                    for (int j = cellData.Length - 1; j >= 0; j--)
                    {
                        cellData[j] = groupVertIDs[cellData[j]];
                    }
                    groupSymmetry.Vertices = cellData;

                    cellData = groupSymmetry.Edges;
                    for (int j = cellData.Length - 1; j >= 0; j--)
                    {
                        if (cellData[j] < 0) { continue; }
                        cellData[j] = groupEdgeIDs[cellData[j]];
                    }
                    groupSymmetry.Edges = cellData;

                    cellData = groupSymmetry.Neighbors;
                    for (int j = cellData.Length - 1; j >= 0; j--)
                    {
                        if (cellData[j] < 0) { continue; }
                        cellData[j] = groupSymmetryIDs[cellData[j]];
                    }
                    groupSymmetry.Neighbors = cellData;

                    cellData = groupSymmetry.Neighbors;
                    for (int j = cellData.Length - 1; j >= 0; j--)
                    {
                        if (cellData[j] < 0) { continue; }
                        cellData[j] = groupSymmetryIDs[cellData[j]];
                    }
                    groupSymmetry.Neighbors = cellData;

                    newSymmetries.Add(groupSymmetry);

                    yield return null;
                }
            }

            symmetries = newSymmetries.ToArray();
            vertices = newVerts.ToArray();
            edges = newEdges.ToArray();

            //And, to round things out, we get our edges and borders straight:
            IEnumerator findBorders = FindBorders(Extents);
            while (findBorders.MoveNext()) { yield return null; }

            yield break;
        }

        public IEnumerator PopulateGroup(ComplexSymmetryGroup[] symmetryGroups, int[] priorities)
        {
            /* We start by sorting by priority. 
             * All lower priority groups have their cells paired, whenever
             * they overlap the dominant groups cells - UNLESS those cells are edge cells. Then the overlapping cells are copied into
             * new hybrid cells that are designed to help the grids merge. The originals are then still removed.
             * When all such groups have been paired, we're left with one list of cells, edges, and neighbors.
             */

            List<SymGroup> symGroups = new List<SymGroup>(symmetryGroups.Length);

            for (int i = symmetryGroups.Length - 1; i >= 0; i--)
            {
                symGroups[i].group = symmetryGroups[i];
                symGroups[i].priority = priorities[i];
            }

            symGroups.Sort((x, y) => x.priority.CompareTo(y.priority));

            for (int i = 0; i < symGroups.Count; i++)
            {
                IEnumerator merger = MergeGroup(symGroups[i].group);
                while (merger.MoveNext()) { yield return null; }
            }

            yield break;
        }


        public override IsoSymmetry GetNeighbor(IsoSymmetry baseSymmetry, int cellID, int neighborID)
        {
            return null;
        }

    }

}
