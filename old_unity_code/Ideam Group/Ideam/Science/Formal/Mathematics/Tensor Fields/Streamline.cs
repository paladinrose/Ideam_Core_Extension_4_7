using System;
using System.Linq;
using System.Collections.Generic;

namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public class Streamline
    {
        private readonly HashSet<TraceVertex> _vertices = new HashSet<TraceVertex>();
        public IEnumerable<TraceVertex> Vertices
        {
            get
            {
                return _vertices;
            }
        }

        public int Count
        {
            get
            {
                return _vertices.Count;
            }
        }

        public TraceVertex First { get; private set; }

        public TraceVertex Last { get; private set; }

        public uint Width { get; set; }

        public TracedRegion Region { get; set; }

        public Streamline(TraceVertex start)
        {
            First = start;
            Last = start;

            _vertices.Add(start);
        }

        internal bool Add(TraceVertex vertex)
        {
            return _vertices.Add(vertex);
        }

        internal bool Remove(TraceVertex vertex)
        {
            return _vertices.Remove(vertex);
        }

        public bool Contains(TraceVertex vertex)
        {
            return _vertices.Contains(vertex);
        }

        internal TracerEdge Extend(TraceVertex endVertex)
        {
            if (!First.Equals(endVertex) && !Add(endVertex))
                throw new ArgumentException("Cannot extend: Streamline already contains vertex", "endVertex");

            //Check that we are not connecting two vertices which are already connected
            var a = Last;
            var b = endVertex;
            if (a.Edges.Any(e => e.A.Equals(a) && e.B.Equals(b) || e.A.Equals(b) && e.B.Equals(a)))
                return null;

            //Extend
            Last = b;
            return TracerEdge.Create(this, a, b);
        }
    }

}