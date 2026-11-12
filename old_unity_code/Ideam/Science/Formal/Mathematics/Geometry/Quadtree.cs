using System;

namespace Ideam.Science.Formal.Mathematics.Geometry
{
    /// <summary>
    /// A 2D  Partitioning Tree
    /// </summary>
    /// <typeparam name="TItem"></typeparam>
    public class Quadtree<TItem> : GeometricTree<TItem, Float2, Bounds2>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="bounds"></param>
        /// <param name="threshold"></param>
        /// <param name="maxDepth"></param>
        public Quadtree(Bounds2 bounds, int threshold, int maxDepth)
            : base(bounds, threshold, maxDepth)
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="container"></param>
        /// <param name="contained"></param>
        /// <returns></returns>
        protected override bool Contains(Bounds2 container, ref Bounds2 contained)
        {
            return container.Contains(contained);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns></returns>
        protected override bool Intersects(Bounds2 a, ref Bounds2 b)
        {
            return a.Intersects(b);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="bound"></param>
        /// <returns></returns>
        protected override Bounds2[] Split(Bounds2 bound)
        {
            var bounds = new Bounds2[4];
            var min = bound.Min;
            var size = (bound.Max - bound.Min) / 2f;

            var i = 0;
            for (var x = 0; x < 2; x++)
            {
                for (var y = 0; y < 2; y++)
                {
                    var positionOffset = size * new Float2(x, y);
                    bounds[i++] = new Bounds2(min + positionOffset, min + size + positionOffset);
                }
            }

            return bounds;
        }
    }
}
