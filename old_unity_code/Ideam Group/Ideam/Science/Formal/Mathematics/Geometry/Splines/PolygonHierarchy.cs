using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Logic.Sets;

namespace Ideam.Science.Formal.Mathematics.Geometry.Splines
{
    [System.Serializable]
    public class PolygonHierarchy : Hierarchy<Spline2>
    {
        [Flags]
        public enum PolygonType {
            Solid = 1,                          // Combines with its parent. 
            PersistentSolid = 2,                // Combines with all its ancestors.
            Hole = 4,                           // Subtracts from its parent.
            PersistentHole = 8,                 // Subtracts from all its ancestors.
            Intersection = 16,                   // Creates an area from each intersection between it and its parent.
            PersistentIntersection = 32,         // Creates an area from each intersection between it and all its ancestors.
            NonIntersection = 64,                // Creates an area from each area that does not intersect between it and its parent.
            PersistentNonIntersection = 128,      // Creates an area from each area that does not intersect between it and all its ancestors.
        }
        
        public FlattenedPolygon[] flattenedPolys;
        

        public PolygonType rootType;
        public PolygonType[] polyTypes;
        public bool extentsDirty;

        public PolygonHierarchy()
        {
            rootObject = Spline2.NewPolygon(3, 2);
            rootObject.name = "Root Polygon";

            for (int i = rootObject.Length; i >= 0; i--)
            {
                rootObject[i] *= 2;
            }
            members = new Spline2[0];
            rootType = PolygonType.Solid;
            polyTypes = new PolygonType[0];
            flattenedPolys = new FlattenedPolygon[0];
            
            genealogies = new Genealogy[0];
            activeStatus = new HierarchyStatus[0];
            //selected = new bool[0];
            extentsDirty = true;
            
            /*
#if UNITY_EDITOR
            showTargets = new bool[0];
            showKids = new bool[0];
            selected = new bool[0];
#endif
            */

        }

        public PolygonHierarchy(PolygonHierarchy original)
        {
            int l = original.Length;
            rootObject = new Spline2(original.rootObject);
            rootType = original.rootType;
            members = new Spline2[l];
            flattenedPolys = new FlattenedPolygon[l];
           
            genealogies = new Genealogy[l];
            polyTypes = new PolygonType[l];
            activeStatus = new HierarchyStatus[l];

            /*
#if UNITY_EDITOR
            showTargets = new bool[l];
            showKids = new bool[l];
            selected = new bool[l];
#endif
            */

            //Debug.Log("Staring dupe creation.");
            for (int i = 0; i < l; i++)
            {
                members[i] = new Spline2(original[i]);
                if (original.flattenedPolys[i] != null)
                {
                    flattenedPolys[i] = new FlattenedPolygon(original.flattenedPolys[i]);
                }
                polyTypes[i] = original.polyTypes[i];
                genealogies[i] = new Genealogy(original.genealogies[i]);
                activeStatus[i] = original.activeStatus[i];

                /*
#if UNITY_EDITOR
                showTargets[i] = original.showTargets[i];
                showKids[i] = original.showKids[i];
                selected[i] = original.selected[i];
#endif
                */

            }
            // Debug.Log("Added polys.");
            if (genealogies.Length > 0)
            {
                SetGeneration(0, genealogies[0].generation);
            }
            //Debug.Log("Generation set.");


            needsFlattened = true;
            extents = original.Extents;
            //Debug.Log("Dupe created!");
        }
        
        public int LineageChangedAt(int id)
        {
            int[]desc =  GetAllDescendants(id);
            
            for(int i = desc.Length-1; i >=0; i--)
            {
                if (flattenedPolys[desc[i]].changed) { return desc[i];}
            }
            if(flattenedPolys[id].changed) { return id;}
            return -1;
        }

        public Bounds2 extents;
        public Bounds2 Extents
        {
            get {
                if (extentsDirty)
                {
                    //Debug.Log("Getting extents on shape: " + name);
                    needsFlattened = true;
                    // Starting at root, going through all children, and making sure the bounds encompass 
                    // all Shapes, but no holes.
                    extents = rootObject.Extents;
                    for (int i = genealogies.Length - 1; i >= 0; i--)
                    {
                        if (polyTypes[i] != PolygonType.Hole && polyTypes[i] != PolygonType.PersistentHole)
                        {
                            
                            extents.Encapsulate(members[i].Extents);
                        }
                    }
                    extentsDirty = false;
                }
                return extents;
            }
        }
        
        Spline2[] flat;
        bool needsFlattened = true;
        public Spline2[] flattenedPolygons
        {
            get
            {
                if (needsFlattened)
                {
                    
                    flat = FlattenHierarchy();
                    needsFlattened = false;
                }
                return flat;
            }
        }

        public void GetClosestPoint(Float2 point, out int polyID, out int pointID)
        {

            int currentID = -10, currentpid = -1;
            int pid = rootObject.GetClosestAnchorPoint(point);
            float currentDist = Float2.Distance(point, rootObject.AnchorPoints[pid]);

            for (int i = Length - 1; i >= 0; i--)
            {
                pid = members[i].GetClosestAnchorPoint(point);
                float dist = Float2.Distance(point, members[i].AnchorPoints[pid]);
                if (dist < currentDist) { currentID = i; currentpid = pid; currentDist = dist; }
            }
            polyID = currentID;
            pointID = currentpid;
        }

        public void Offset(Float2 offset)
        {
            rootObject.Offset(offset);
            for (int i = Length - 1; i >= 0; i--)
            {
                members[i].Offset(offset);
                flattenedPolys[i].changed = true;
            }
            extentsDirty = true;
        }

        /// <summary>
        /// Adds a new polygon to our hierarchy, attached directly to the root object.
        /// </summary>
        /// <param name="id"></param>
        public override void Add(int id = -1)
        {
            Spline2 newPoly = Spline2.NewPolygon();
            newPoly.name = "New Polygon";
            Add(newPoly, id);
            List<PolygonType> nPT = new List<PolygonType>(polyTypes);
            List<FlattenedPolygon> nf = new List<FlattenedPolygon>(flattenedPolys);
            
            nf.Add(new FlattenedPolygon());
            nPT.Add(PolygonType.Solid);

            flattenedPolys = nf.ToArray();
            polyTypes = nPT.ToArray();

            extentsDirty = true;
        }

        /// <summary>
        /// Removes a polygon from the hierarchy
        /// </summary>
        /// <param name="id">The polygon to remove</param>
        /// <param name="includeKids">Whether or not to remove the children of the removed object.</param>
        public override int[] Remove(int id, bool includeKids = true)
        {

            int[] thoseThatGo = GetAllDescendants(id);
            //Debug.Log("Removing " + id);
            int[] newIDS = base.Remove(id);
            //Debug.Log("Core remove complete. Remove polytype.");
            List<PolygonType> nPT = new List<PolygonType>(polyTypes);
            List<FlattenedPolygon> nf = new List<FlattenedPolygon>(flattenedPolys);
            
            if (includeKids)
            {
                bool[] tRem = new bool[polyTypes.Length];
                tRem[id] = true;
                for (int i = thoseThatGo.Length - 1; i >= 0; i--)
                {
                    tRem[thoseThatGo[i]] = true;
                }
                
                for (int i = polyTypes.Length - 1; i >= 0; i--)
                {
                    if (tRem[i])
                    {
                        nPT.RemoveAt(i);
                        nf.RemoveAt(i);
                        
                    }

                }
            } else
            {
                nPT.RemoveAt(id);
                nf.RemoveAt(id);
               
            }
            polyTypes = nPT.ToArray();
            flattenedPolys = nf.ToArray();
            return newIDS;
        }
        public override int[] Remove(int[] ids, bool includeKids = true)
        {
            extentsDirty = true;
            return base.Remove(ids, includeKids);
        }

        public override void SetAsRoot(int id)
        {

            rootType = polyTypes[id];
            base.SetAsRoot(id);

        }
        public bool ContainsPoint(Float2 point, ref int id, bool simple = true, bool checkActiveStatus = false, bool solidsOnly = false)
        {
            //First, is it in our bounds?
            
            if (Extents.Contains(point))
            {
                bool contains = false;

                
                Spline2[] f = flattenedPolygons;
                
                for (int i = f.Length - 1; i >= 0; i--)
                {
                    if (f[i].ContainsPoint(point))
                    {
                        contains = true;
                        break;
                    }
                }

                id = -1;
                if (simple)
                {
                    return contains;
                }
                else
                {
                    if (contains) {
                        //First, let's just collect a raw list of any polys that:
                        // A. DO contain the point and
                        // B. ARE active in the hierarchy and
                        // C. ARE solids
                        List<int> hits = new List<int>();
                        for (int i = Length - 1; i >= 0; i--)
                        {
                            if (checkActiveStatus && activeStatus[i] == HierarchyStatus.Disabled) { continue; }

                            if (solidsOnly && (polyTypes[i] == PolygonType.Hole || polyTypes[i] == PolygonType.PersistentHole)) { continue; }

                            if (members[i].ContainsPoint(point))
                            {
                                hits.Add(i);
                            }

                        }
                        //Now, let's find the one with the highest id(s)
                        List<int> highestHits = new List<int>();
                        int highest = -10;
                        for (int i = hits.Count - 1; i >= 0; i--)
                        {
                            if (genealogies[hits[i]].generation > highest)
                            {
                                highestHits = new List<int>();
                                highestHits.Add(hits[i]);
                                highest = genealogies[hits[i]].generation;
                            }
                            else if (genealogies[hits[i]].generation == highest)
                            {
                                highestHits.Add(hits[i]);
                            }
                        }
                        if (highest == -10)
                        {
                            //Check the root. It SHOULD be required to be the root, if we made it to this situation, but...
                            return rootObject.ContainsPoint(point);
                        }
                        else
                        {
                            // If we have more than one, we pick whichever one has the closest point between verts and center.
                            //Otherwise, we're done.
                            if (highestHits.Count == 1)
                            {
                                id = highestHits[0];

                            }
                            else if (highestHits.Count > 0)
                            {
                                float close = float.MaxValue;
                                int cl = -1;
                                for (int i = highestHits.Count - 1; i >= 0; i--)
                                {
                                    float dist = Float2.Distance(members[highestHits[i]].Extents.Center, point);
                                    if (dist < close)
                                    {
                                        cl = highestHits[i];
                                        close = dist;
                                    }

                                    for (int j = members[highestHits[i]].Length - 1; j >= 0; j--)
                                    {
                                        dist = Float2.Distance(members[highestHits[i]][j], point);
                                        if (dist < close)
                                        {
                                            cl = highestHits[i];
                                            close = dist;
                                        }
                                    }
                                }
                                id = cl;
                            }
                            else { return false; }
                            return true;
                        }
                    }
                }
            }
            return false;
        }


        /// <summary>
        /// Returns a collection of Polygons in a flat, non-overlapping layer from the whole hierarchy.
        /// </summary>
        /// <returns></returns>
        public Spline2[] FlattenHierarchy()
        {
            //Debug.Log("Flattening Hierarchy");
            List<Spline2> final = new List<Spline2>();
            
            for (int i = rootChildren.Length - 1; i >= 0; i--)
            {
                int rc = rootChildren[i];
                
                int lca = LineageChangedAt(rc);
                int cr = 0;
                while(lca != -1 && cr < 5)
                {
                    FlattenAtID(lca);
                    cr++;
                    lca = LineageChangedAt(rc);
                    //Debug.Log(lca);
                }
            }
            
            if (rootType == PolygonType.Hole)
            {
                for (int i = 0; i < rootChildren.Length; i++)
                {
                    for(int j = 0; j < flattenedPolys[i].polys.Length; j++)
                    if (flattenedPolys[i].types[j] == PolygonType.Solid || 
                        flattenedPolys[i].types[j] == PolygonType.PersistentSolid)
                    {
                        final.Add(flattenedPolys[i].polys[j]);
                    }
                }
            }
            // If the root is a Solid, apply all of the first generations just as we have been.
            else
            {
                // We treat it as a solid if it's not a hole. Just to prevent hijinks.
                final.Add(rootObject);
                for (int i = 0; i< rootChildren.Length; i++)
                {
                    int finCount = final.Count;
                    for (int j = 0; j < flattenedPolys[i].polys.Length; j++)
                    {
                        for (int k = 0; k < finCount; k++)
                        {

                            //Debug.Log("Before switch: " + dupe.polyTypes[i].ToString());
                            Spline2[] flattenResults = new Spline2[0];
                            switch (flattenedPolys[i].types[j])
                            {
                                case PolygonType.Solid:
                                    //Debug.Log("Combining finals." + i);
                                    flattenResults = final[k].CombinePolygons(flattenedPolys[i].polys[j]);
                                    break;
                                case PolygonType.Hole:
                                    flattenResults = final[k].SubtractPolygons(flattenedPolys[i].polys[j]);
                                    break;
                                case PolygonType.Intersection:
                                    flattenResults = final[k].GetIntersectingPolygons(flattenedPolys[i].polys[j]);
                                    break;
                                case PolygonType.NonIntersection:
                                    flattenResults = final[k].GetNonIntersectingPolygons(flattenedPolys[i].polys[j]);
                                    break;
                            }

                            //Debug.Log("After switch");
                            // All of our results for these four will result in a collection of Solids.
                            // S0, we replace our source polygon with the first result, then add the remainder.
                            if (flattenResults.Length > 0)
                            {

                                final[k] = flattenResults[0];

                                for (int cp = 1; cp < flattenResults.Length; cp++)
                                {
                                    //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.

                                    float clockWise = 0f;
                                    Float2[] fR = flattenResults[cp].SmoothedPoints;
                                    for (int p = fR.Length - 1; p >= 0; p--)
                                    {
                                        if (p != fR.Length - 1)
                                        {
                                            clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                        }
                                        else
                                        {
                                            clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                        }
                                    }
                                    if (clockWise > 0f)
                                    {
                                        final.Add(flattenResults[cp]);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return final.ToArray();
        }

        public Spline2[] GetFlattenedPolygons(int[] ids)
        {
            List<Spline2> final = new List<Spline2>();
            List<PolygonType> fTypes = new List<PolygonType>();

            bool[] usedID = new bool[ids.Length];
            
            
            bool includeRoot = false;

            int numTrueRef = -99;
            
            while (usedID.NumberTrue(ref numTrueRef) < ids.Length)
            {
                int lowestgen = YoungestGeneration;
                List<int> lowest = new List<int>();
                for (int i = ids.Length - 1; i >= 0; i--)
                {
                    if (ids[i] != -10 && !usedID[i])
                    {
                        if (genealogies[ids[i]].generation < lowestgen)
                        {
                            lowest = new List<int>();
                            lowestgen = genealogies[ids[i]].generation;
                            lowest.Add(i);
                        }
                        else if (genealogies[ids[i]].generation == lowestgen)
                        {
                            lowest.Add(i);
                        }
                    }
                    else
                    {
                        includeRoot = true;
                    }
                }

                for (int i = 0; i < lowest.Count; i++)
                {
                    int l = lowest[i];
                    usedID[l] = true;

                    List<Genealogy> family = new List<Genealogy>();
                    family.Add(genealogies[l]);
                    List<int> childCounter = new List<int>(),
                        famIDs = new List<int>();
                    childCounter.Add(0);
                    famIDs.Add(l);

                    List<Spline2[]> toCollapse = new List<Spline2[]>();
                    List<PolygonType[]> cTypes = new List<PolygonType[]>();

                    Genealogy current;
                    int c;

                    while (family.Count > 0)
                    {
                        int famc = family.Count - 1;
                        current = family[famc];
                        c = childCounter[famc];

                        if (c < current.childIds.Length)
                        {
                            childCounter[famc]++;
                            family.Add(genealogies[current.childIds[c]]);
                            childCounter.Add(0);
                            famIDs.Add(current.childIds[c]);
                            toCollapse.Add(null);
                            cTypes.Add(null);
                        }
                        else
                        {
                            for (int k = ids.Length - 1; k >= 0; k--)
                            {
                                if (ids[k] == famIDs[famc])
                                {
                                    //Ok, at this point, we have
                                    //verified that this thing
                                    //either doesn't have kids
                                    // or we've already processed
                                    // them. This one is also on
                                    // our ids list, so we're taking
                                    // its raw polygon data and
                                    // merging it with any
                                    // polygons that have come
                                    // from its kids.

                                    List<Spline2> currentParent = new List<Spline2>();
                                    currentParent.Add(members[ids[k]]);
                                    List<PolygonType> currentType = new List<PolygonType>();
                                    currentType.Add(polyTypes[ids[k]]);
                                    List<int> toRemove = new List<int>();

                                    for (int jj = 0; jj < toCollapse[famc].Length; jj++)
                                    {

                                        Spline2[] flattenResults = new Spline2[0];

                                        int finCount = currentParent.Count;

                                        for (int kk = 0; kk < finCount; kk++)
                                        {
                                            switch (currentType[kk])
                                            {
                                                case PolygonType.Solid:

                                                    switch (cTypes[famc][jj])
                                                    {
                                                        case PolygonType.Solid:
                                                            flattenResults = currentParent[kk].CombinePolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.Hole:
                                                            flattenResults = currentParent[kk].SubtractPolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.Intersection:
                                                            flattenResults = currentParent[kk].GetIntersectingPolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.NonIntersection:
                                                            flattenResults = currentParent[kk].GetIntersectingPolygons(toCollapse[famc][jj]);
                                                            break;
                                                    }
                                                    // All of our results for these four will result in a collection of Solids.
                                                    // S0, we replace our source polygon with the first result, then add the remainder.
                                                    if (flattenResults.Length > 0)
                                                    {
                                                        currentParent[kk] = flattenResults[0];
                                                        for (int cp = 1; cp < flattenResults.Length; cp++)
                                                        {

                                                            float clockWise = 0f;
                                                            Float2[] fR = flattenResults[cp].SmoothedPoints;
                                                            for (int p = fR.Length - 1; p >= 0; p--)
                                                            {
                                                                if (p != fR.Length - 1)
                                                                {
                                                                    clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                                                }
                                                                else
                                                                {
                                                                    clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                                                }
                                                            }
                                                            if (clockWise > 0f)
                                                            {
                                                                currentParent.Add(flattenResults[cp]);
                                                                currentType.Add(currentType[kk]);
                                                            }
                                                            else
                                                            {
                                                                System.Array.Reverse(fR);
                                                                currentParent.Add(new Spline2(fR));
                                                                currentType.Add(PolygonType.Hole);
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        toRemove.Add(kk);
                                                    }
                                                    break;

                                                case PolygonType.Hole:

                                                    switch (cTypes[famc][jj])
                                                    {
                                                        case PolygonType.Solid:
                                                            //Subtract, then forward shape to our forwards list.
                                                            flattenResults = currentParent[kk].SubtractPolygons(toCollapse[famc][jj]);
                                                            //Debug.Log("Forwarding Shape.");

                                                            //forwards.Add(toCollapse[famc][jj]);
                                                            //forwardTypes.Add(PolygonType.Solid);
                                                            break;
                                                        case PolygonType.Hole:
                                                            flattenResults = currentParent[kk].CombinePolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.Intersection:
                                                            flattenResults = currentParent[kk].GetIntersectingPolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.NonIntersection:
                                                            flattenResults = currentParent[kk].GetIntersectingPolygons(toCollapse[famc][jj]);
                                                            break;
                                                    }
                                                    if (flattenResults.Length > 0)
                                                    {
                                                        // There's a risk I could end up trying to use a Hole as the original shape, but 
                                                        // unmarked. IF I'm getting such a screwup, I need to make sure this first
                                                        // guy is counterclockwise and then sort through until I find the first that
                                                        // is a solid, not a hole.
                                                        currentParent[kk] = flattenResults[0];

                                                        for (int cp = 1; cp < flattenResults.Length; cp++)
                                                        {
                                                            //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.
                                                            float clockWise = 0f;
                                                            Float2[] fR = flattenResults[cp].SmoothedPoints;
                                                            for (int p = fR.Length - 1; p >= 0; p--)
                                                            {
                                                                if (p != fR.Length - 1)
                                                                {
                                                                    clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                                                }
                                                                else
                                                                {
                                                                    clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                                                }
                                                            }
                                                            if (clockWise > 0f)
                                                            {
                                                                currentParent.Add(flattenResults[cp]);
                                                                currentType.Add(currentType[kk]);
                                                            }
                                                            else
                                                            {
                                                                System.Array.Reverse(fR);
                                                                currentParent.Add(new Spline2(fR));
                                                                currentType.Add(PolygonType.Solid);
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        toRemove.Add(k);
                                                    }
                                                    break;

                                                case PolygonType.Intersection:

                                                    switch (cTypes[famc][jj])
                                                    {
                                                        case PolygonType.Solid:
                                                            flattenResults = currentParent[kk].CombinePolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.Hole:
                                                            flattenResults = currentParent[kk].SubtractPolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.Intersection:
                                                            flattenResults = currentParent[kk].GetIntersectingPolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.NonIntersection:
                                                            flattenResults = currentParent[kk].GetIntersectingPolygons(toCollapse[famc][jj]);
                                                            break;
                                                    }
                                                    if (flattenResults.Length > 0)
                                                    {
                                                        currentParent[kk] = flattenResults[0];

                                                        for (int cp = 1; cp < flattenResults.Length; cp++)
                                                        {
                                                            //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.
                                                            float clockWise = 0f;
                                                            Float2[] fR = flattenResults[cp].SmoothedPoints;
                                                            for (int p = fR.Length - 1; p >= 0; p--)
                                                            {
                                                                if (p != fR.Length - 1)
                                                                {
                                                                    clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                                                }
                                                                else
                                                                {
                                                                    clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                                                }
                                                            }
                                                            if (clockWise > 0f)
                                                            {
                                                                currentParent.Add(flattenResults[cp]);
                                                                currentType.Add(currentType[kk]);
                                                            }
                                                            else
                                                            {
                                                                System.Array.Reverse(fR);
                                                                currentParent.Add(new Spline2(fR));
                                                                currentType.Add(PolygonType.Hole);
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        toRemove.Add(kk);
                                                    }
                                                    break;

                                                case PolygonType.NonIntersection:


                                                    switch (cTypes[famc][jj])
                                                    {
                                                        case PolygonType.Solid:
                                                            flattenResults = currentParent[kk].CombinePolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.Hole:
                                                            flattenResults = currentParent[kk].SubtractPolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.Intersection:
                                                            flattenResults = currentParent[kk].GetIntersectingPolygons(toCollapse[famc][jj]);
                                                            break;
                                                        case PolygonType.NonIntersection:
                                                            flattenResults = currentParent[kk].GetIntersectingPolygons(toCollapse[famc][jj]);
                                                            break;
                                                    }

                                                    if (flattenResults.Length > 0)
                                                    {
                                                        currentParent[kk] = flattenResults[0];

                                                        for (int cp = 1; cp < flattenResults.Length; cp++)
                                                        {
                                                            //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.
                                                            float clockWise = 0f;
                                                            Float2[] fR = flattenResults[cp].SmoothedPoints;
                                                            for (int p = fR.Length - 1; p >= 0; p--)
                                                            {
                                                                if (p != fR.Length - 1)
                                                                {
                                                                    clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                                                }
                                                                else
                                                                {
                                                                    clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                                                }
                                                            }
                                                            if (clockWise > 0f)
                                                            {
                                                                currentParent.Add(flattenResults[cp]);
                                                                currentType.Add(currentType[kk]);
                                                            }
                                                            else
                                                            {
                                                                System.Array.Reverse(fR);
                                                                currentParent.Add(new Spline2(fR));
                                                                currentType.Add(PolygonType.Hole);
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        toRemove.Add(kk);
                                                    }
                                                    break;
                                            }
                                        }
                                    }


                                    for (int cp = currentParent.Count - 1; cp >= 0; cp--)
                                    {
                                        for (int tR = toRemove.Count - 1; tR >= 0; tR--)
                                        {
                                            if (toRemove[tR] == cp)
                                            {
                                                currentParent.RemoveAt(cp);
                                            }
                                        }
                                    }
                                    toCollapse[famc] = currentParent.ToArray();
                                    cTypes[famc] = currentType.ToArray();
                                }
                            }
                            if (famc > 0 && toCollapse[famc].Length > 0)
                            {
                                List<Spline2> combo = new List<Spline2>(toCollapse[famc - 1]);
                                combo.AddRange(toCollapse[famc]);
                                toCollapse[famc - 1] = combo.ToArray();
                                List<PolygonType> typo = new List<PolygonType>(cTypes[famc - 1]);
                                typo.AddRange(cTypes[famc]);
                                cTypes[famc - 1] = typo.ToArray();
                            }
                            else if (famc == 0)
                            {
                                final.AddRange(toCollapse[famc]);
                                fTypes.AddRange(cTypes[famc]);
                            }
                            family.RemoveAt(famc);
                            childCounter.RemoveAt(famc);
                            famIDs.RemoveAt(famc);
                            toCollapse.RemoveAt(famc);
                            cTypes.RemoveAt(famc);
                        }
                    }
                }
            }
            if (includeRoot)
            {
                if (rootType != PolygonType.Hole && rootType != PolygonType.PersistentHole)
                {
                    // We treat it as a solid if it's not a hole. Just to prevent hijinks.
                    List<Spline2> finale = new List<Spline2>();
                    finale.Add(rootObject);
                    for (int i = 0; i < final.Count; i++)
                    {
                        int finCount = finale.Count;
                        for(int j = 0; j < finCount; j++)
                        { 
                            Spline2[] flattenResults = new Spline2[0];
                            switch (fTypes[i])
                            {
                                case PolygonType.Solid:
                                    //Debug.Log("Combining finals." + i);
                                    flattenResults = finale[j].CombinePolygons(final[i]);
                                    break;
                                case PolygonType.Hole:
                                    flattenResults = finale[j].SubtractPolygons(final[i]);
                                    break;
                                case PolygonType.Intersection:
                                    flattenResults = finale[j].GetIntersectingPolygons(final[i]);
                                    break;
                                case PolygonType.NonIntersection:
                                    flattenResults = finale[j].GetNonIntersectingPolygons(final[i]);
                                    break;
                            }

                            //Debug.Log("After switch");
                            // All of our results for these four will result in a collection of Solids.
                            // S0, we replace our source polygon with the first result, then add the remainder.
                            if (flattenResults.Length > 0)
                            {

                                finale[j] = flattenResults[0];

                                for (int cp = 1; cp < flattenResults.Length; cp++)
                                {
                                    //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.

                                    float clockWise = 0f;
                                    Float2[] fR = flattenResults[cp].SmoothedPoints;
                                    for (int p = fR.Length - 1; p >= 0; p--)
                                    {
                                        if (p != fR.Length - 1)
                                        {
                                            clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                        }
                                        else
                                        {
                                            clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                        }
                                    }
                                    if (clockWise > 0f)
                                    {
                                        finale.Add(flattenResults[cp]);
                                    }
                                }
                            }
                        }
                    }
                final = finale;
                }
            }
            
            //prune any holes.
            for (int i = final.Count - 1; i >= 0; i--)
            {
                if (fTypes[i] ==PolygonType.Hole || fTypes[i] == PolygonType.PersistentHole)
                {
                    final.RemoveAt(i);
                }
            }
            
            return final.ToArray();
        }

        public void FlattenAtID(int id)
        {
            IEnumerator fID = FlattenAtID(id, 0);
            while (fID.MoveNext()) { }
        }
        public IEnumerator FlattenAtID(int id, int cycles = 0)
        {
            // I. Create a Spline2 List that includes JUST the parent polygon.

            List<Spline2> currentParent = new List<Spline2>();
            currentParent.Add(members[id]);
            List<PolygonType> currentType = new List<PolygonType>();
            currentType.Add(polyTypes[id]);
            // This is for shapes that are being directly forwarded. Other kids and their effects shouldn't be
            // applied. These should become siblings of the original parent.
            //List<Spline2> forwards = new List<Spline2>();
            //List<PolygonType> forwardTypes = new List<PolygonType>();

            
            // II. Go through each child. As we resolve it, we put the flatten results into it so that each
            //     additional child is applied to all the results collectively.
            List<int> toRemove = new List<int>();
            for (int j = 0; j < genealogies[id].childIds.Length; j++)
            {
                int cID = genealogies[id].childIds[j];
                for(int fc = 0; fc < flattenedPolys[cID].polys.Length; fc++)
                {
                    yield return null;

                    Spline2[] flattenResults = new Spline2[0];

                    int finCount = currentParent.Count;

                    for (int k = 0; k < finCount; k++)
                    {
                        
                        switch (currentType[k])
                        {
                            case PolygonType.Solid:

                                switch (flattenedPolys[cID].types[fc])
                                {
                                    case PolygonType.Solid:
                                        flattenResults = currentParent[k].CombinePolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.Hole:
                                        flattenResults = currentParent[k].SubtractPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.Intersection:
                                        flattenResults = currentParent[k].GetIntersectingPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.NonIntersection:
                                        flattenResults = currentParent[k].GetIntersectingPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                }
                                // All of our results for these four will result in a collection of Solids.
                                // S0, we replace our source polygon with the first result, then add the remainder.
                                if (flattenResults.Length > 0)
                                {
                                    currentParent[k] = flattenResults[0];

                                    for (int cp = 1; cp < flattenResults.Length; cp++)
                                    {
                                        yield return null; 
                                        //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.

                                        float clockWise = 0f;
                                        Float2[] fR = flattenResults[cp].SmoothedPoints;
                                        for (int p = fR.Length - 1; p >= 0; p--)
                                        {
                                            if (p != fR.Length - 1)
                                            {
                                                clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                            }
                                            else
                                            {
                                                clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                            }
                                        }
                                        if (clockWise > 0f)
                                        {
                                            currentParent.Add(flattenResults[cp]);
                                            currentType.Add(currentType[k]);
                                        }
                                        else
                                        {
                                            System.Array.Reverse(fR);
                                            currentParent.Add(new Spline2(fR));
                                            currentType.Add(PolygonType.Hole);
                                        }
                                    }
                                }
                                else
                                {
                                    toRemove.Add(k);
                                }
                                break;

                            case PolygonType.Hole:

                                switch (flattenedPolys[cID].types[fc])
                                {
                                    case PolygonType.Solid:
                                        //Subtract, then forward shape to our forwards list.
                                        flattenResults = currentParent[k].SubtractPolygons(flattenedPolys[cID].polys[fc]);
                                        //Debug.Log("Forwarding Shape.");

                                        //forwards.Add(flattenedPolys[cID].polys[fc]);
                                        //forwardTypes.Add(PolygonType.Solid);
                                        break;
                                    case PolygonType.Hole:
                                        flattenResults = members[id].CombinePolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.Intersection:
                                        flattenResults = members[id].GetIntersectingPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.NonIntersection:
                                        flattenResults = members[id].GetIntersectingPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                }
                                if (flattenResults.Length > 0)
                                {
                                    // There's a risk I could end up trying to use a Hole as the original shape, but 
                                    // unmarked. IF I'm getting such a screwup, I need to make sure this first
                                    // guy is counterclockwise and then sort through until I find the first that
                                    // is a solid, not a hole.
                                    currentParent[k] = flattenResults[0];

                                    for (int cp = 1; cp < flattenResults.Length; cp++)
                                    {
                                        //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.
                                        //if (cycler.Tick()) { yield return null; }
                                        
                                        float clockWise = 0f;
                                        Float2[] fR = flattenResults[cp].SmoothedPoints;
                                        for (int p = fR.Length - 1; p >= 0; p--)
                                        {
                                            if (p != fR.Length - 1)
                                            {
                                                clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                            }
                                            else
                                            {
                                                clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                            }
                                        }
                                        if (clockWise > 0f)
                                        {
                                            currentParent.Add(flattenResults[cp]);
                                            currentType.Add(currentType[k]);
                                        }
                                        else
                                        {
                                            System.Array.Reverse(fR);
                                            currentParent.Add(new Spline2(fR));
                                            currentType.Add(PolygonType.Solid);
                                        }
                                    }
                                }
                                else
                                {
                                    toRemove.Add(k);
                                }
                                break;

                            case PolygonType.Intersection:

                                switch (flattenedPolys[cID].types[fc])
                                {
                                    case PolygonType.Solid:
                                        flattenResults = members[id].CombinePolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.Hole:
                                        flattenResults = members[id].SubtractPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.Intersection:
                                        flattenResults = members[id].GetIntersectingPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.NonIntersection:
                                        flattenResults = members[id].GetIntersectingPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                }
                                if (flattenResults.Length > 0)
                                {
                                    currentParent[k] = flattenResults[0];

                                    for (int cp = 1; cp < flattenResults.Length; cp++)
                                    {
                                        //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.
                                        //if (cycler.Tick()) { yield return null; }
                                        
                                        float clockWise = 0f;
                                        Float2[] fR = flattenResults[cp].SmoothedPoints;
                                        for (int p = fR.Length - 1; p >= 0; p--)
                                        {
                                            if (p != fR.Length - 1)
                                            {
                                                clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                            }
                                            else
                                            {
                                                clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                            }
                                        }
                                        if (clockWise > 0f)
                                        {
                                            currentParent.Add(flattenResults[cp]);
                                            currentType.Add(currentType[k]);
                                        }
                                        else
                                        {
                                            System.Array.Reverse(fR);
                                            currentParent.Add(new Spline2(fR));
                                            currentType.Add(PolygonType.Hole);
                                        }
                                    }
                                }
                                else
                                {
                                    toRemove.Add(k);
                                }
                                break;

                            case PolygonType.NonIntersection:


                                switch (flattenedPolys[cID].types[fc])
                                {
                                    case PolygonType.Solid:
                                        flattenResults = members[id].CombinePolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.Hole:
                                        flattenResults = members[id].SubtractPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.Intersection:
                                        flattenResults = members[id].GetIntersectingPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                    case PolygonType.NonIntersection:
                                        flattenResults = members[id].GetIntersectingPolygons(flattenedPolys[cID].polys[fc]);
                                        break;
                                }
                                if (flattenResults.Length > 0)
                                {
                                    currentParent[k] = flattenResults[0];

                                    for (int cp = 1; cp < flattenResults.Length; cp++)
                                    {
                                        //Check to see if it's counter-clockwise. If it is, it's a hole. Add to forwards.
                                        //if (cycler.Tick()) { yield return null; }
                                        
                                        float clockWise = 0f;
                                        Float2[] fR = flattenResults[cp].SmoothedPoints;
                                        for (int p = fR.Length - 1; p >= 0; p--)
                                        {
                                            if (p != fR.Length - 1)
                                            {
                                                clockWise += (fR[p + 1].x - fR[p].x) * (fR[p].y + fR[p + 1].y);
                                            }
                                            else
                                            {
                                                clockWise += (fR[0].x - fR[p].x) * (fR[0].y + fR[p].y);
                                            }
                                        }
                                        if (clockWise > 0f)
                                        {
                                            currentParent.Add(flattenResults[cp]);
                                            currentType.Add(currentType[k]);
                                        }
                                        else
                                        {
                                            System.Array.Reverse(fR);
                                            currentParent.Add(new Spline2(fR));
                                            currentType.Add(PolygonType.Hole);
                                        }
                                    }
                                }
                                else
                                {
                                    toRemove.Add(k);
                                }
                                break;
                        }
                    }
                }
                
            }
            flattenedPolys[id].changed = false;
            //Debug.Log("Flattened kids. Finalizing.");
            for (int cp = currentParent.Count - 1; cp >= 0; cp--)
            {
                for (int tR = toRemove.Count - 1; tR >= 0; tR--)
                {
                    if (toRemove[tR] == cp)
                    {
                        currentParent.RemoveAt(cp);
                        yield return null;
                    }
                }
            }

            flattenedPolys[id].polys = currentParent.ToArray();
            flattenedPolys[id].types = currentType.ToArray(); 
            yield break;
        }

        public PolygonHierarchy Detach(int id, bool removeOriginal = true)
        {
            PolygonHierarchy newH = new PolygonHierarchy();
            newH.Add(members[id]);
            //it's now newH[0]
            List<int> ids = new List<int>(GetAllDescendants(id));
            ids.Insert(0, id);

            for (int i = 1; i < ids.Count; i++)
            {
                newH.Add(members[ids[i]]);
            }

            //At this point, newH has the same number of entry as ids, and each one corresponds. I can check
            //the members id slot to find its original id number.
            for (int i = 1; i < ids.Count; i++)
            {
                int newParentID = genealogies[ids[i]].parentID;
                for (int j = ids.Count - 1; j >= 0; j--)
                {
                    if (ids[j] == newParentID)
                    {
                        newParentID = j;
                        break;
                    }
                }
                newH.SetParent(i, newParentID);
            }
            newH.SetAsRoot(0);
            if (removeOriginal)
            {
                Remove(id);
            }
            return newH;
        }

        public int GetID(Spline2 p)
        {
            for (int i = Length - 1; i >= 0; i--)
            {
                if (members[i] == p) { return i; }
            }
            return -1;
        }

        public Float2[] GetConvexPerimeter()
        {
            Spline2[] p = FlattenHierarchy();
            return p.GetConvexPerimeter();
        }

        public void Triangulate(out Float2[] triangleVertices, out int[] triangleIndices)
        {
            List<Float2> verts = new List<Float2>();
            List<int> indices = new List<int>();
            Spline2[] flattened = FlattenHierarchy();
            //Debug.Log(name + " has " + flattened.Length + " flattened polys.");
            for (int i = 0; i < flattened.Length; i++)
            {

                Float2[] pts = flattened[i].SmoothedPoints;
                int[] tris = flattened[i].Triangulate();
                int[] amendedTris = new int[tris.Length];

                // Go through all the points. If they have not been added to our new verts list, add it. Get its index.
                // Go through its tris and any original indices, set the same slot to the new index on amendedTris.
                // When complete with all points, add all of AmendedTris, as is, to indices.
                for (int p = 0; p < pts.Length; p++)
                {
                    bool found = false;
                    int id = p;
                    for (int ap = verts.Count - 1; ap >= 0; ap--)
                    {
                        if (verts[ap] == pts[p])
                        {
                            found = true; id = ap; break;
                        }
                    }
                    if (!found)
                    {
                        id = verts.Count; verts.Add(pts[p]);
                    }

                    for (int tc = tris.Length - 1; tc >= 0; tc--)
                    {
                        if (tris[tc] == p)
                        {
                            amendedTris[tc] = id;
                        }
                    }
                }
                for (int t = 0; t < tris.Length; t++)
                {
                    indices.Add(amendedTris[t]);
                }
            }

            triangleVertices = verts.ToArray();
            triangleIndices = indices.ToArray();
        }
        /*

        //#if UNITY_EDITOR
        public Color color = Color.green;
        public Rect labelRect;
        public bool showFlattened = false;
        public virtual void SetupEditor()
        {
            Float2 lC = HandleUtility.WorldToGUIPoint(new Vector3(Extents.xMax, Extents.yMax, 0));
            labelRect = new Rect(lC, GUI.skin.button.CalcSize(new GUIContent(name)));
        }

        public virtual void DrawPolygonSelectionView()
        {
            Color hand = Handles.color;
            if (rootSelected) {
                rootObject.DrawSplineView(rootObject.color, Color.white, rootObject.pointSelection);
                rootObject.DrawPointViewSelection();
            }
            else {
                rootObject.DrawSplineView(Color.gray, Color.gray, new bool[rootObject.pointSelection.Length]);
            }


            for (int i = Length - 1; i >= 0; i--) {
                if (selected[i]) { members[i].DrawSplineView(members[i].color, Color.white, members[i].pointSelection); }
                else { members[i].DrawSplineView(Color.gray, Color.gray, new bool[members[i].pointSelection.Length]); }

                if (selected[i])
                {
                    //Debug.Log("Drawing points for polygon " + i);
                    members[i].DrawPointViewSelection();
                }
            }
            if (showFlattened)
            {
                DrawFlattened();
            }
            Handles.color = hand;
        }
        public void DrawFlattened()
        {
            if (flat != null)
            {
                for (int i = flat.Length - 1; i >= 0; i--)
                {
                    flat[i].DrawSplineView(Color.cyan, Color.cyan, new bool[flat[i].Length + 1]);

                }
            }
        }

        public override void DrawHierarchyLine(int id, Rect pos)
        {
            Color c = GUI.backgroundColor;
            if (id == mouseHolder || id == holdingID)
            {
                //drawBox.x = pos.x;
                //drawBox.width = pos.width;

                if (mouseHolder != holdingID)
                {
                    if (id == mouseHolder) { GUI.backgroundColor = new Color(c.r * 1.1f, c.g * 1.1f, c.b * 1.1f, c.a); }
                    else { GUI.backgroundColor = new Color(c.r * 0.8f, c.g * 0.8f, c.b * 0.8f, c.a); }
                }
                else
                {
                    GUI.backgroundColor = new Color(c.r * 1.25f, c.g * 1.25f, c.b * 1.25f, c.a);
                }
                GUI.Box(drawBox, GUIContent.none);

            }
            else if (selected[id])
            {
                GUI.backgroundColor = new Color(c.r * 0.75f, c.g * 0.75f, c.b * 1.25f, c.a);
                GUI.Box(drawBox, GUIContent.none);
            }
            GUI.backgroundColor = c;
            drawBox.x = pos.x + currentIndent;
            drawBox.width = lineHeight;

            showKids[id] = EditorGUI.Foldout(drawBox, showKids[id], new GUIContent());
            //Selectable name box to select entries in our hierarchy.
            drawBox.x += lineHeight;
            drawBox.width = pos.width - lineHeight * 2 - currentIndent;
            drawBox.width *= 0.5f;
            // drawBox.width = GUILayoutUtility.GetRect(new GUIContent(genealogies[id].name), GUI.skin.label).width;
            if (GUI.Button(drawBox, id.ToString(), CruxGUI.labelStyle))
            {
                //Check which key is down. If control is held,
                if (!Event.current.control)
                {
                    selected = new bool[selected.Length];
                    selected[id] = true;
                    rootSelected = false;
                }
                else
                {
                    selected[id] = !selected[id];
                }
                if (selected[id])
                {
                    latestSelection = id;
                }
                Event.current.Use();
            }
            drawBox.x += drawBox.width;
            drawBox.width *= 0.5f;
            polyTypes[id] = (PolygonType)EditorGUI.EnumPopup(drawBox, polyTypes[id]);


            // Check box for Show Contents.
            drawBox.x += drawBox.width;
            //drawBox.width = lineHeight * 2;
            HierarchyStatus hs = (HierarchyStatus)EditorGUI.EnumPopup(drawBox, activeStatus[id]);
            if (hs != activeStatus[id])
            {
                activeStatus[id] = hs;
                Event.current.Use();
            }

            drawBox.x += drawBox.width;
            drawBox.width = lineHeight;

            bool doShow = CruxGUI.CruxToggle(drawBox, showTargets[id], 7, 6);
            if (doShow != showTargets[id])
            {
                showTargets[id] = doShow;
                Event.current.Use();
            }
            drawBox.x = pos.x;
            drawBox.width = pos.width;
            drawBox.y += lineHeight;

        }

        public override void DrawHierarchyContent(int id, SerializedProperty targ, Rect pos)
        {
            Color c = GUI.backgroundColor, p = members[id].color, f = CruxGUI.fontColor;
            if ((p.r > 0.9f || p.g > 0.9f || p.b > 0.9f) || ((p.r + p.g + p.b) >=2f))
            {
                CruxGUI.fontColor = Color.black;
            }else
            {
                CruxGUI.fontColor = Color.white;
            }
            
            GUI.backgroundColor = p;
            base.DrawHierarchyContent(id, targ, pos);
            GUI.backgroundColor = c;
            CruxGUI.fontColor = f;
        }

        public virtual void DrawPolygonSelectionLabels()
        {
            Color c = GUI.color;

            if (rootSelected) {
                rootObject.DrawPointNumbers();
                GUI.color = Color.white; }
            else
            { GUI.color = rootObject.color; }

            Rect iconRect = new Rect(rootObject.labelRect.xMin - rootObject.labelRect.height * 0.5f,
                rootObject.labelRect.y, rootObject.labelRect.height, rootObject.labelRect.height);
            Texture polyIcon = CruxGUI.icons[13];
            switch (rootType)
            {
                
                case PolygonType.Hole:
                case PolygonType.PersistentHole:
                    polyIcon = CruxGUI.icons[14];
                    break;
                case PolygonType.Intersection:
                case PolygonType.PersistentIntersection:
                    polyIcon = CruxGUI.icons[15];
                    break;
                case PolygonType.NonIntersection:
                case PolygonType.PersistentNonIntersection:
                    polyIcon = CruxGUI.icons[16];
                    break;
            }
            if (GUI.Button(iconRect, polyIcon, GUI.skin.box))
            {
                if (!Event.current.control) { rootSelected = true; selected = new bool[Length];  }
                else { rootSelected = !rootSelected; }
                if (rootSelected) { latestSelection = -10; }
            }
            //GUILayout.BeginArea(rootObject.labelRect);
            if (GUI.Button(rootObject.labelRect, rootObject.name))
            {
                if (!Event.current.control) { rootSelected = true; selected = new bool[Length]; }
                else { rootSelected = !rootSelected; }
                if (rootSelected) { latestSelection = -10; }
            }
            //GUILayout.EndArea();

            for (int h = Length - 1; h >= 0; h--)
            {
                if (selected[h]) {
                    members[h].DrawPointNumbers();
                    GUI.color = Color.white; }
                else
                { GUI.color = members[h].color; }
                
                iconRect = new Rect(members[h].labelRect.xMin-members[h].labelRect.height*0.5f,
                    members[h].labelRect.y, members[h].labelRect.height, members[h].labelRect.height);
                polyIcon = CruxGUI.icons[11];
                switch (polyTypes[h])
                {
                    case PolygonType.Solid:
                    case PolygonType.PersistentSolid:
                        polyIcon = CruxGUI.icons[13];
                        break;
                    case PolygonType.Hole:
                    case PolygonType.PersistentHole:
                        polyIcon = CruxGUI.icons[14];
                        break;
                    case PolygonType.Intersection:
                    case PolygonType.PersistentIntersection:
                        polyIcon = CruxGUI.icons[15];
                        break;
                    case PolygonType.NonIntersection:
                    case PolygonType.PersistentNonIntersection:
                        polyIcon = CruxGUI.icons[16];
                        break;
                }
                if (GUI.Button(iconRect, polyIcon, GUI.skin.box))
                {
                    if (!Event.current.control) { rootSelected = false;  selected = new bool[Length]; selected[h] = true; }
                    else { selected[h] = !selected[h]; }
                    if (selected[h]) { latestSelection = h; }
                }
               // GUILayout.BeginArea(members[h].labelRect);
                if (GUI.Button(members[h].labelRect, members[h].name))
                {
                    if (!Event.current.control) { rootSelected = false; selected = new bool[Length]; selected[h] = true; }
                    else { selected[h] = !selected[h]; }
                    if (selected[h]) { latestSelection = h; }
                }
                //GUILayout.EndArea();
            }
            GUI.color = c;
        }
        

        public virtual Float2 DrawMoveHandle(Color c)
        {
            Handles.color = c;
            float hs = HandleUtility.GetHandleSize(Extents.center);
            Float2 vec1 = Handles.FreeMoveHandle(extents.max, Quaternion.identity, hs*0.08f, Vector3.zero, Handles.CircleHandleCap);
            if (vec1 != extents.max)
            {
                vec1 -= extents.max;
                
                return vec1;
            }
            return Float2.zero;
        }
        
        public virtual bool DrawButton(Color c)
        {
            Handles.color = c;
            float hs = HandleUtility.GetHandleSize(Extents.center);
            return Handles.Button(Extents.center, Quaternion.identity, hs*0.08f, hs*0.1f, Handles.CubeHandleCap);
        }

        public virtual Float2 DrawPolygonMoveHandles(ref int id, Color c)
        {
            Float2 offset = Float2.zero;
            id = -1;

            if (rootSelected)
            {
                offset = rootObject.DrawMoveHandle(c);
                if(offset != Float2.zero)
                {
                    id = -10;
                }
            }
            if (id != -10)
            {
                for (int i = Length - 1; i >= 0; i--)
                {
                    if (selected[i])
                    {
                        offset = members[i].DrawMoveHandle(c);
                        if (offset != Float2.zero)
                        {
                            id = i;
                            break;
                        }
                    }
                }
            }
            return offset;
        }

        public virtual bool DrawPolygonButtons(ref int id, Color c)
        {
            id = -1;
            if (rootSelected)
            {
                Color cc = c;
                if (c == color) { cc = rootObject.color; }
                if (rootObject.DrawButtonHandle(cc))
                {
                    id = -10;
                    return true;
                }
            }
            for (int i = Length - 1; i >= 0; i--)
            {
                Color cc = c;
                if (c == color) { cc = members[i].color;}
                if (selected[i] && members[i].DrawButtonHandle(cc))
                {
                    id = i;
                    return true;
                }
            }
            return false;
        }
//#endif
        */

    }
    
    [System.Serializable]
    public class FlattenedPolygon
    {
        public Spline2[] polys = new Spline2[0];
        public PolygonHierarchy.PolygonType[] types = new 
            PolygonHierarchy.PolygonType[0];
        public bool changed;

        public FlattenedPolygon()
        {
            polys = new Spline2[0];
            types = new PolygonHierarchy.PolygonType[0];
            changed = true;
        }

        public FlattenedPolygon(FlattenedPolygon p)
        {
            if (p.polys != null)
            {
                polys = new Spline2[p.polys.Length];
                for (int i = p.polys.Length - 1; i >= 0; i--)
                {
                    polys[i] = new Spline2(p.polys[i]);
                }
                types = p.types;
                changed = p.changed;
            }

        }
    }
}