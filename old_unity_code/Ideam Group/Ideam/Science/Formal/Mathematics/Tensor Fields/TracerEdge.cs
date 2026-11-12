using System;

namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public class TracerEdge
    {
        private readonly TraceVertex _a;
        public TraceVertex A
        {
            get
            {
                return _a;
            }
        }

        private readonly TraceVertex _b;
        public TraceVertex B
        {
            get
            {
                return _b;
            }
        }

        private readonly Float2 _direction;
        public Float2 Direction { get { return _direction; } }

        private readonly Streamline _streamline;
        public Streamline Streamline
        {
            get
            {
                return _streamline;
            }
        }

        private TracerEdge(Streamline stream, TraceVertex a, TraceVertex b)
        {

            _a = a;
            _b = b;
            _streamline = stream;
            _direction = (b.Position - a.Position).Normalize();
        }

        /// <summary>
        /// Replace this edge with two new ones A -> Mid -> B
        /// </summary>
        /// <param name="mid"></param>
        /// <param name="aMid"></param>
        /// <param name="midB"></param>
        public TraceVertex Split(TraceVertex mid, out TracerEdge aMid, out TracerEdge midB)
        {
            
            if (!_streamline.Add(mid))
                throw new InvalidOperationException("Invalid Split Operation: Streamline already contains mid vertex");

            if (!A.Remove(this))
                throw new InvalidOperationException("Invalid Split Operation: Vertex does not contain edge");
            if (!B.Remove(this))
                throw new InvalidOperationException("Invalid Split Operation: Vertex does not contain edge");

            aMid = Create(Streamline, A, mid);
            midB = Create(Streamline, mid, B);

            return mid;
        }

        public static TracerEdge Create(Streamline stream, TraceVertex a, TraceVertex b)
        {
            
            var e = new TracerEdge(stream, a, b);
            a.Add(e);
            b.Add(e);

            return e;
        }
    }

}