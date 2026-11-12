using System;
using System.Linq;
using System.Xml.Linq;
using System.Collections.Generic;

namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public class Network
    {
        private readonly TraceVertex[] _vertices;
        public IEnumerable<TraceVertex> Vertices
        {
            get
            {
                return _vertices;
            }
        }

        public Network(IEnumerable<TraceVertex> vertices)
        {
            _vertices = vertices.ToArray();
        }


        public string ToSvg(IEnumerable<TracedRegion> regions = null)
        {
            var g = new XElement("g",
                new XAttribute("transform", "translate(10, 10)")
            );

            var min = new Float2(float.MaxValue, float.MaxValue);
            var max = new Float2(float.MinValue, float.MinValue);
            foreach (var vertex in _vertices)
            {
                foreach (var edge in vertex.Edges)
                {
                    if (Equals(edge.A, vertex))
                    {
                        g.Add(new XElement("line",
                            new XAttribute("x1", edge.A.Position.x),
                            new XAttribute("y1", edge.A.Position.y),
                            new XAttribute("x2", edge.B.Position.x),
                            new XAttribute("y2", edge.B.Position.y),
                            new XAttribute("style", string.Format("stroke:rgb(0,0,0);stroke-width:{0};stroke-linecap:round", Math.Max(1, edge.Streamline.Width)))
                        ));

                        min = new Float2(Math.Min(min.x, edge.A.Position.x), Math.Min(min.y, edge.A.Position.y));
                        max = new Float2(Math.Max(max.x, edge.A.Position.x), Math.Max(max.y, edge.A.Position.y));
                    }
                }
            }

            if (regions != null)
            {
                int i = 0;
                foreach (var region in regions)
                {
                    var points = region.Vertices
                        .Select(a => string.Format("{0},{1}", a.x, a.y));
                    var path = string.Join(" ", points);

                    float hue = ((0.618033988749895f * i) % 1) * 360;

                    g.Add(new XElement("polygon",
                        new XAttribute("points", path),
                        new XAttribute("style", string.Format("fill:hsla({0},100%,50%,0.1);", hue))
                    ));

                    i++;
                }
            }

            var svg = new XElement("svg", new XAttribute("width", max.x + 20), new XAttribute("height", max.y + 20));
            svg.AddFirst(g);

            var doc = new XDocument();
            doc.Add(svg);

            return doc.ToString();
        }
    }
}