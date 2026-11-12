using System;

namespace Ideam.Science.Formal.Mathematics.Geometry
{
    /// <summary>
    /// 3D Space Partitioning Tree
    /// </summary>
    /// <typeparam name="TItem"></typeparam>
    public class Octree<TItem> : GeometricTree<TItem, Float3, Bounds3>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="bounds"></param>
        /// <param name="threshold"></param>
        /// <param name="maxDepth"></param>
        public Octree(Bounds3 bounds, int threshold, int maxDepth)
            : base(bounds, threshold, maxDepth)
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="container"></param>
        /// <param name="contained"></param>
        /// <returns></returns>
        protected override bool Contains(Bounds3 container, ref Bounds3 contained)
        {
            return container.Contains(contained);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns></returns>
        protected override bool Intersects(Bounds3 a, ref Bounds3 b)
        {
            return a.Intersects(b);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="bound"></param>
        /// <returns></returns>
        protected override Bounds3[] Split(Bounds3 bound)
        {
            var bounds = new Bounds3[8];
            var min = bound.Min;
            var size = (bound.Max - bound.Min) / 2f;

            var i = 0;
            for (var x = 0; x < 2; x++)
            {
                for (var y = 0; y < 2; y++)
                {
                    for (var z = 0; z < 2; z++)
                    {
                        var positionOffset = size * new Float3(x, y, z);
                        bounds[i++] = new Bounds3(min + positionOffset, min + size + positionOffset);
                    }
                }
            }

            return bounds;
        }
    }
}
