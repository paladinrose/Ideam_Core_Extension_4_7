using System;
using System.Collections.Generic;

namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public class TracedRegion
    {
        private readonly Float2[] _vertices;
        public IEnumerable<Float2> Vertices
        {
            get
            {

                return _vertices;
            }
        }

        private readonly Float2 _min;
        public Float2 Min
        {
            get
            {
                return _min;
            }
        }

        private readonly Float2 _max;
        public Float2 Max
        {
            get
            {
                return _max;
            }
        }

        public TracedRegion(List<Float2> vertices)
        {
            _vertices = vertices.ToArray();

            _min = new Float2(float.MaxValue);
            _max = new Float2(float.MinValue);
            foreach (var vertex in vertices)
            {
                _min = new Float2(Math.Min(vertex.x, _min.x), Math.Min(vertex.y, _min.y));
                _max = new Float2(Math.Max(vertex.x, _max.x), Math.Max(vertex.y, _max.y));
            }
        }


        #region winding
        public bool IsClockwise()
        {
            //http://stackoverflow.com/a/1165943/108234

            float sum = 0;

            for (int i = 0; i < _vertices.Length; i++)
            {
                var a = _vertices[i];
                var b = _vertices[(i + 1) % _vertices.Length];

                var n = (b.x - a.x) * (b.y - a.y);

                sum += n;
            }

            return sum > 0;
        }

        public TracedRegion Flip()
        {
            Array.Reverse(_vertices);
            return this;
        }
        #endregion

        #region point in polygon
        /// <summary>
        /// Determine whether given 2D point lies within 
        /// the polygon.
        /// 
        /// Written by Jeremy Tammik, Autodesk, 2009-09-23, 
        /// based on code that I wrote back in 1996 in C++, 
        /// which in turn was based on C code from the 
        /// article "An Incremental Angle Point in Polygon 
        /// Test" by Kevin Weiler, Autodesk, in "Graphics 
        /// Gems IV", Academic Press, 1994.
        /// 
        /// Copyright (C) 2009 by Jeremy Tammik. All 
        /// rights reserved.
        /// 
        /// This code may be freely used. Please preserve 
        /// this comment.
        /// </summary>
        public bool PointInPolygon(Float2 point)
        {
            unsafe
            {
                fixed (Float2* arr = &_vertices[0])
                {
                    Float2* pointPtr = &point;

                    //http://thebuildingcoder.typepad.com/blog/2010/12/point-in-polygon-containment-algorithm.html

                    // initialize
                    int quad = GetQuadrant(*arr, *pointPtr);
                    int angle = 0;

                    // loop on all vertices of polygon
                    int n = _vertices.Length;
                    for (int i = 0; i < n; ++i)
                    {
                        Float2* vertex = arr + i;
                        Float2* nextVertex = arr + ((i + 1 < n) ? i + 1 : 0);

                        // calculate quadrant and delta from last quadrant
                        int nextQuad = GetQuadrant(*nextVertex, *pointPtr);
                        int delta = (nextQuad - quad);

                        AdjustDelta(ref delta, vertex, nextVertex, pointPtr);

                        // add delta to total angle sum
                        angle = (angle + delta);

                        // increment for next step
                        quad = nextQuad;
                    }

                    // complete 360 degrees (angle of + 4 or -4 ) 
                    // means inside

                    return (angle == +4) || (angle == -4);

                    // odd number of windings rule:
                    // if (angle & 4) return INSIDE; else return OUTSIDE;
                    // non-zero winding rule:
                    // if (angle != 0) return INSIDE; else return OUTSIDE;
                }
            }
        }

        private static unsafe void AdjustDelta(ref int delta, Float2* vertex, Float2* nextVertex, Float2* p)
        {
            switch (delta)
            {
                // make quadrant deltas wrap around:
                case 3: delta = -1; break;
                case -3: delta = 1; break;
                // check if went around point cw or ccw:
                case 2:
                case -2:
                    if (XIntercept(vertex, nextVertex, p->y) > p->x)
                        delta = -delta;
                    break;
                default:
                    break;
            }
        }

        private static unsafe float XIntercept(Float2* p, Float2* q, float y)
        {
            if (Math.Abs(p->y - q->y) < float.Epsilon)
                throw new ArgumentException("unexpected horizontal segment");

            return q->x - ((q->y - y) * ((p->x - q->x) / (p->y - q->y)));
        }

        private static int GetQuadrant(Float2 vertex, Float2 p)
        {
            return ((vertex.x > p.x)
                 ? ((vertex.y > p.y) ? 0 : 3)
                 : ((vertex.y > p.y) ? 1 : 2));
        }
        #endregion
    }

}