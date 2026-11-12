using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Mathematics
{
    public enum XYToXYZ
    {
        XY, XZ, YZ
    }

    public static class Mathematics
    {
        public static int NumberTrue(this bool[] check, ref int singleTrueID)
        {
            int t = 0;
            for (int i = check.Length - 1; i >= 0; i--)
            {
                if (check[i]) { t++; singleTrueID = i; }
            }
            if (t > 1) { singleTrueID = -1; }
            return t;
        }

        #region Extension Methods
        public static Float2[] Normalize(this Float2[] floats)
        {
            Float2[] normalized = new Float2[floats.Length];
            Float2 min = Float2.zero;
            Float2 max = Float2.zero;
            for (int i = floats.Length - 1; i >= 0; i--)
            {
                if (floats[i].x < min.x) { min.x = floats[i].x; }
                if (floats[i].x > max.x) { max.x = floats[i].x; }
                if (floats[i].y < min.y) { min.y = floats[i].y; }
                if (floats[i].y > max.y) { max.y = floats[i].y; }
            }
            Float2 size = max - min;
            Float2 scl = new Float2(1f / size.x, 1f / size.y),
                offset = Float2.zero;

            if (min.x < 0) { offset.x = min.x * scl.x * -1f; }
            if (min.y < 0) { offset.y = min.y * scl.y * -1f; }

            for (int i = floats.Length - 1; i >= 0; i--)
            {
                normalized[i] = new Float2((floats[i].x * scl.x) + offset.x, (floats[i].y * scl.y) + offset.y);
            }

            return normalized;
        }
        #endregion
    }
}
