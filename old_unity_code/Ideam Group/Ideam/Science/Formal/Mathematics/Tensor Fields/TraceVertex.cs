using System.Collections.Generic;

using System.Threading;
namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public class TraceVertex
    {
        internal readonly Float2 PositionField;
        public Float2 Position { get { return PositionField; } }

        private readonly List<TracerEdge> _edges = new List<TracerEdge>();

        public IEnumerable<TracerEdge> Edges
        {
            get
            {
                return _edges;
            }
        }

        public int EdgeCount { get { return _edges.Count; } }

        private static int _nextHash = int.MinValue;
        private readonly int _hash;

        public TraceVertex(Float2 position)
        {
            PositionField = position;

            _hash = Interlocked.Increment(ref _nextHash);
        }

        internal void Add(TracerEdge edge)
        {
            _edges.Add(edge);
        }

        internal bool Remove(TracerEdge edge)
        {
            return _edges.Remove(edge);
        }

        public override int GetHashCode()
        {
            return _hash;
        }

        public override bool Equals(object obj)
        {
            return ReferenceEquals(obj, this);
        }
    }
}