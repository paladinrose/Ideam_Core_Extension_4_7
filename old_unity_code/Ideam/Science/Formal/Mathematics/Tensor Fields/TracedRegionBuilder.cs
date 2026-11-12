using System;
using System.Linq;
using System.Collections.Generic;


namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public class TracedRegionBuilder
    {
        private readonly HashSet<TracerEdge> _unprocessed;
        private readonly HashSet<KeyValuePair<bool, TracerEdge>> _halfProcessed = new HashSet<KeyValuePair<bool, TracerEdge>>();

        public TracedRegionBuilder(IEnumerable<TraceVertex> vertices)
        {
            
            _unprocessed = new HashSet<TracerEdge>(vertices.SelectMany(v => v.Edges));
        }

        public IEnumerable<TracedRegion> Regions()
        {
            
            var regions = new List<TracedRegion>();

            //Empty out these two collections (it's larger, let's keep it as small as possible)
            while (_unprocessed.Count > 0 || _halfProcessed.Count > 0)
            {
                TracerEdge edge;
                bool direction;
                if (_halfProcessed.Count > 0)
                {
                    var e = _halfProcessed.First();
                    edge = e.Value;
                    direction = e.Key;
                }
                else
                {
                    edge = _unprocessed.First();
                    direction = true;
                }

                var boundary = WalkRegionBoundary(edge, direction, _unprocessed, _halfProcessed);
                if (boundary != null)
                    regions.Add(boundary);
            }

            //Sort the regions into a consistent order
            //Even small variations in min and max should result in exactly the same order each time
            return regions.OrderBy(a => Math.Round(a.Min.x))
                          .ThenBy(a => Math.Round(a.Min.y))
                          .ThenBy(a => Math.Round(a.Max.x))
                          .ThenBy(a => Math.Round(a.Max.y));
        }

        private static TracedRegion WalkRegionBoundary(TracerEdge start, bool direction, ISet<TracerEdge> unprocessed, ISet<KeyValuePair<bool, TracerEdge>> halfProcessed)
        {
            
            //We only want to trace boundary streamlines (width zero) on the inside, not the outside
            //Skip this streamline if it is a clockwise boundary streamline (and update collections)
            if (start.Streamline.Width == 0)
            {
                halfProcessed.Remove(new KeyValuePair<bool, TracerEdge>(true, start));
                halfProcessed.Remove(new KeyValuePair<bool, TracerEdge>(false, start));
                unprocessed.Remove(start);
                return null;
            }

            var points = new List<TraceVertex>();

            var e = start;
            do
            {
                //If this edge was previously unprocessed, the other side has not been touched yet, queue it up to be processed
                if (unprocessed.Remove(e))
                    halfProcessed.Add(new KeyValuePair<bool, TracerEdge>(!direction, e));
                else if (!halfProcessed.Remove(new KeyValuePair<bool, TracerEdge>(direction, e)))
                    return null;

                //Accumulate edges
                var p = (direction ? e.B : e.A);
                points.Add(p);

                //Step to next edge
                e = WalkNextEdge(e, ref direction);

                if (e == null)
                    return null;

            } while (e != start);

            RemoveDeadEnds(points);

            if (points.Count >= 3)
                return new TracedRegion(points.Select(a => a.Position).ToList());

            return null;
        }

        private static void RemoveDeadEnds(IList<TraceVertex> points)
        {
            
            //it's possible for a region to follow up a dead end road
            //This will manifest as a point both preceded and followed by the same vertex
            //We want to remove the vertex, and one of the two neighbours and keep doing this until no more are left

            for (var i = 0; i < points.Count; i++)
            {
                //If we have too few points, clear the collection and give up
                if (points.Count <= 3)
                {
                    points.Clear();
                    return;
                }

                //Get the two points we're interested in (next and previous)
                var iPrev = (i + points.Count - 1) % points.Count;
                var prev = points[iPrev];

                var iVert = (i + points.Count) % points.Count;
                var vert = points[iVert];

                var iNext = (i + 1) % points.Count;
                var next = points[iNext];

                if (prev.Equals(next))
                {
                    points.RemoveAt(iVert);
                    points.RemoveAt(iPrev);
                    i -= 2;
                }
                else if (vert.Equals(next))
                {
                    points.RemoveAt(iVert);
                    i -= 1;
                }
            }
        }

        private static TracerEdge WalkNextEdge(TracerEdge edge, ref bool direction)
        {
            
            var v = direction ? edge.B : edge.A;
            var d = edge.Direction * (direction ? 1 : -1);

            var edgeCount = v.EdgeCount;

            switch (edgeCount)
            {
                case 0:
                    return null;

                case 1:
                    //Dead end! Reverse down other side of same edge
                    direction = !direction;
                    return edge;

                case 2:
                    //Straight path! Continue down next edge in direction
                    var next = v.Edges.Single(e => e != edge);
                    direction = next.A.Equals(v);
                    return next;

            }

            //Split choice
            var minAngle = float.MaxValue;
            var chosenDirection = true;
            TracerEdge chosenEdge = null;

            foreach (var item in v.Edges)
            {
                if (item.Equals(edge))
                    continue;

                var bDir = item.A.Equals(v);
                var vDir = item.Direction * (bDir ? 1 : -1);
                var angle = SignedAngle(d, vDir);

                if (angle < minAngle)
                {
                    minAngle = angle;
                    chosenDirection = bDir;
                    chosenEdge = item;
                }
            }

            direction = chosenDirection;
            return chosenEdge;
        }

        private static float SignedAngle(Float2 a, Float2 b)
        {
            //http://stackoverflow.com/a/21486462/108234
            var dot = Float2.Dot(a, b);
            var cross = Float2.Cross(a, b);
            return (float)Math.Atan2(cross, dot);
        }
    }

}