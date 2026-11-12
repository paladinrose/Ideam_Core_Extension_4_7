using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;

using UnityEngine;

namespace Ideam.Science.Formal.Mathematics.Geometry.Splines
{
    public enum CurveType { Linear, CatmullRom, CubicBezier, Step, None }
    public enum PointSearchType { Any, Before, After }

    
    [Serializable]
    public struct SplineSection
    {
        public CurveType curveType;
        public float weight, length;
    }

    [Serializable]
    public struct IntSplineSection
    {
        public CurveType curveType;
        public int weight, length;
    }

    [Serializable]
    public struct IntPoint1
    {
        public Int point, backHandle, foreHandle;
        public bool useBackHandle, useForeHandle;
        
    }

    [Serializable]
    public struct IntPoint2
    {
        public Int2 point, backHandle, foreHandle;
        public bool useBackHandle, useForeHandle;
    }

    [Serializable]
    public struct IntPoint3
    {
        public Int3 point, backHandle, foreHandle;
        public bool useBackHandle, useForeHandle;
    }

    [Serializable]
    public struct IntPoint4
    {
        public Int4 point, backHandle, foreHandle;
        public bool useBackHandle, useForeHandle;
    }


    [Serializable]
    public struct Point1
    {
        public Float point, backHandle, foreHandle;
        public bool useBackHandle, useForeHandle;

    }

    [Serializable]
    public struct Point2
    {
        public Float2 point, backHandle, foreHandle;
        public bool useBackHandle, useForeHandle;
    }

    [Serializable]
    public struct Point3
    {
        public Float3 point, backHandle, foreHandle;
        public bool useBackHandle, useForeHandle;
    }

    [Serializable]
    public struct Point4
    {
        public Float4 point, backHandle, foreHandle;
        public bool useBackHandle, useForeHandle;
    }


   
    public static class Splines
    {
        #region Curve Methods
        public static float CatmullRom(float t, float pa, float pb, float pc, float pd)
        {
            return 0.5f * ((-pa + 3f * pb - 3f * pc + pd) * (t * t * t) +
                (2f * pa - 5f * pb + 4f * pc - pd) * (t * t) +
                (-pa + pc) * t + (2f * pb));
        }

        public static float CubicBezier(float t, float pa, float pb, float pc, float pd)
        {
            float u = 1f - t;

            return pa * (u * u * u) +
                   pb * 3f * (u * u) * t +
                   pc * 3f * u * (t * t) +
                   pd * (t * t * t);
        }

        #endregion

        #region Spline1 Extension Methods
        public static Float[] GetPoints(this Point1[] points)
        {
            Float[] p = new Float[points.Length];
            for (int i = points.Length - 1; i >= 0; i--)
            {
                p[i] = points[i].point;
            }
            return p;
        }
        #endregion

        #region Spline2 Extension Methods

        public static Float2[] GetPoints(this Point2[] points)
        {
            Float2[] p = new Float2[points.Length];
            for (int i = points.Length-1; i >= 0; i--)
            {
                p[i] = points[i].point;
            }
            return p;
        }

        public static Float2[] GetBackHandles(this Point2[] points)
        {
            Float2[] p = new Float2[points.Length];
            for (int i = points.Length-1; i >= 0; i--)
            {
                p[i] = points[i].backHandle;
            }
            return p;
        }

        public static Float2[] GetForeHandles(this Point2[] points)
        {
            Float2[] p = new Float2[points.Length];
            for (int i = points.Length-1; i >= 0; i--)
            {
                p[i] = points[i].foreHandle;
            }
            return p;
        }

        public static Bounds2 GetExtents(this Spline2[] polys)
        {
            Bounds2 r = new Bounds2(polys[0].Extents);
            for (int i = 1; i < polys.Length; i++)
            {
                r.Encapsulate(polys[i].Extents);
            }
            return r;
        }

        public static Float2 Minimum(this Spline2[] group)
        {
            Float2 minimum = group[group.Length - 1].Min;
            for (int i = group.Length - 2; i >= 0; i--)
            {

                if (group[i].Min.x < minimum.x)
                {
                    minimum.x = group[i].Min.x;
                }
                if (group[i].Min.y < minimum.y)
                {
                    minimum.y = group[i].Min.y;
                }
            }
            return minimum;
        }

        public static Float2 SmoothMinimum(this Spline2[] group)
        {
            Float2 minimum = group[group.Length - 1].SmoothMin;
            for (int i = group.Length - 2; i >= 0; i--)
            {

                if (group[i].SmoothMin.x < minimum.x)
                {
                    minimum.x = group[i].SmoothMin.x;
                }
                if (group[i].SmoothMin.y < minimum.y)
                {
                    minimum.y = group[i].SmoothMin.y;
                }
            }
            return minimum;
        }

        public static Float2 Maximum(this Spline2[] group)
        {
            Float2 maximum = group[group.Length - 1].Max;
            for (int i = group.Length - 2; i >= 0; i--)
            {

                if (group[i].Max.x < maximum.x)
                {
                    maximum.x = group[i].Max.x;
                }
                if (group[i].Max.y < maximum.y)
                {
                    maximum.y = group[i].Max.y;
                }
            }
            return maximum;
        }

        public static Float2 SmoothMaximum(this Spline2[] group)
        {
            Float2 maximum = group[group.Length - 1].SmoothMax;
            for (int i = group.Length - 2; i >= 0; i--)
            {

                if (group[i].SmoothMax.x < maximum.x)
                {
                    maximum.x = group[i].SmoothMax.x;
                }
                if (group[i].SmoothMax.y < maximum.y)
                {
                    maximum.y = group[i].SmoothMax.y;
                }
            }
            return maximum;
        }


        public static Float2[] RegularPolygon(int sides = 3, float radius = 0.5f, float angleOffset = 0f)
        {
            float d2r = Float.DegToRad;
            Float2[] poly = new Float2[sides];
            float angle = d2r * 360f / sides;
            float ao = d2r * angleOffset;
            for (int i = 1; i <= sides; i++)
            {
                poly[i - 1] = new Float2(radius * (float)Math.Cos(angle * i + ao), radius * (float)Math.Sin(angle * i + ao));
            }

            return poly;
        }

        public static Float2[] RegularStar(int sides = 3, float pointRadius = 0.5f, float innerRadius = 0.1f, float angleOffset = 0f)
        {
            int s = sides * 2;
            Float2[] poly = new Float2[s];
            float angle = Float.DegToRad * 360f / s;
            float ao = Float.DegToRad * angleOffset;
            bool inner = false;

            for (int i = 1; i <= s; i++)
            {
                if (!inner) { poly[i - 1] = new Float2(pointRadius * (float)Math.Cos(angle * i + ao), pointRadius * (float)Math.Sin(angle * i + ao)); }
                else { poly[i - 1] = new Float2(innerRadius * (float)Math.Cos(angle * i + ao), innerRadius * (float)Math.Sin(angle * i + ao)); }
                inner = !inner;
            }

            return poly;
        }

        #endregion

        #region Polygon Extensions

        public static Spline2 GetPolygon(this Bounds2 rect)
        {
            Spline2 poly = new Spline2(new Float2[] { rect.Min, new Float2(rect.Min.x, rect.Max.y), rect.Max, new Float2(rect.Max.x, rect.Min.y) });
            return poly;
        }

        public static int ContainsPoint(this Spline2[] polys, Float2 point)
        {
            int id = -1;
            for (int i = polys.Length - 1; i >= 0; i--)
            {
                if (polys[i].ContainsPoint(point)) { return i; }
            }
            return id;
        }

        public static Float Area(this Spline2[] group)
        {
            Float totalVolume = 0;

            //This calls for all my old polygon tools. I'm confident they're around somewhere in my vaults, and I'll dig them out and update them. But not now.
            //For now, here's a super-stupid rough-shot!!
            for(int i =0; i < group.Length; i++)
            {
                totalVolume += group[i].Area();
            }
            return totalVolume;
        }

        public static Spline2[] Normalize(this Spline2[] group)
        {
            Float2 min = group.SmoothMinimum();
            Float2 max = group.SmoothMaximum();
            Float2 offset = min;
            Float2 scalar = max - min;

            Spline2[] newGroup = new Spline2[group.Length];

            offset *= -1;
            if (scalar.x == 0) { scalar.x = 1f; }
            else { scalar.x = 1f / scalar.x; }
            if (scalar.y == 0) { scalar.y = 1f; }
            else { scalar.y = 1f / scalar.y; }

            for (int h = group.Length - 1; h >= 0; h--)
            {
                Point2[] points;
                group[h].GetAnchorPoints(out points);

                for (int i = points.Length - 1; i >= 0; i--)
                {
                    points[i].point += offset;
                    points[i].point *= scalar;
                    points[i].foreHandle *= scalar;
                    points[i].backHandle *= scalar;
                }
                newGroup[h] = new Spline2();
                newGroup[h].name = group[h].name;
                newGroup[h].SetAnchorPoints(points);
            }
            return newGroup;
        }

        public static Spline2[] Scale(this Spline2[] group, float scalar)
        {
            Spline2[] newGroup = new Spline2[group.Length];
            for (int i = group.Length - 1; i >= 0; i--)
            {
                newGroup[i] = new Spline2(group[i]);
                newGroup[i].Scale(scalar);
            }
            return newGroup;
        }

        public static Float2[] GetConvexPerimeter(this Spline2[] polys)
        {
            List<Float2> perimeterPoints = new List<Float2>();
            int startingPoly = -1, startingPoint = -1;
            //1. Find our top most point. If two of them tie, get the left-most of the two.
            Bounds2 totalBounds = new Bounds2();
            Float2 exMin = Float2.zero, exMax = Float2.zero;
            for (int i = polys.Length - 1; i >= 0; i--)
            {

                Bounds2 pi = polys[i].Extents;
                if (pi.Min.x < exMin.x)
                {
                    exMin.x = pi.Min.x;
                }
                if (pi.Min.y < exMin.y)
                {
                    exMin.y = pi.Min.y;
                }
                if (pi.Max.x > exMax.x)
                {
                    exMax.x = pi.Max.x;
                }
                if (pi.Max.y > exMax.y || (pi.Max.y == exMax.y && pi.Min.x < exMin.x))
                {
                    exMax.y = pi.Max.y;
                    startingPoly = i;
                }

            }
            totalBounds.SetMinMax(exMin, exMax);

            // Okay, now we get our starting point. The polygon's Top parameter is the id 
            // number of the top (or, top left most, in case of ties) point.
            startingPoint = polys[startingPoly].Top;

            int currentPoly = startingPoly, currentPoint = startingPoint, phase = 1, nextPoint, bestP;
            float bestAngle;
            perimeterPoints.Add(polys[currentPoly][startingPoint]);
            //Until we find our way back to that starting point(a,b) we keep moving.
            while (startingPoint != currentPoint || startingPoly != currentPoly)
            {

                switch (phase)
                {
                    // On top, moving right.
                    case 1:
                        // 1. Get the next clockwise point from where we currently are.
                        nextPoint = currentPoint + 1;
                        if (nextPoint >= polys[currentPoly].Length)
                        {
                            nextPoint -= polys[currentPoly].Length;
                        }

                        // 2. Check its y value against the other polygons' Extents.yMax.
                        int leftRightMost = -1;
                        float lrm = float.MaxValue;

                        bestP = -1;
                        bestAngle = 360;
                        for (int i = polys.Length - 1; i >= 0; i--)
                        {
                            if (i != currentPoly)
                            {

                                //  - If we have other polygons with a greater yMax, 
                                //    we take the left-most of them that's also equal to right of our current point. (NOT next point)
                                //    A. We go through the points of this other polygon. 
                                //    B. We find the angles between our current point and the points in our new poly and find the 
                                //    smallest angle, the smallest righthand turn we can make, and we call it next point.
                                if (polys[currentPoly][nextPoint].y <= polys[i].Extents.Max.y &&
                                    polys[i].Extents.Min.x >= polys[currentPoly][currentPoint].x)
                                {

                                    if (polys[i].Extents.Min.x < lrm)
                                    {
                                        leftRightMost = i;
                                        lrm = polys[i].Extents.Min.x;
                                        for (int j = polys[i].Length - 1; j >= 0; j--)
                                        {
                                            float ang = Float2.Angle(polys[currentPoly][currentPoint], polys[i][j]);
                                            if (ang < bestAngle) { bestAngle = ang; bestP = j; }
                                        }
                                    }
                                    else if (polys[i].Extents.Min.x == lrm)
                                    {
                                        for (int j = polys[i].Length - 1; j >= 0; j--)
                                        {
                                            float ang = Float2.Angle(polys[currentPoly][currentPoint], polys[i][j]);
                                            if (ang < bestAngle)
                                            {
                                                leftRightMost = i;
                                                bestAngle = ang; bestP = j;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (leftRightMost >= 0)
                        {
                            perimeterPoints.Add(polys[currentPoly][currentPoint]);
                            currentPoly = leftRightMost;
                            currentPoint = bestP;
                        }
                        else
                        {
                            //  - If no other poly's met our criteria, we accept this next point.
                            perimeterPoints.Add(polys[currentPoly][currentPoint]);
                            currentPoint = nextPoint;
                        }
                        if (polys[currentPoly][currentPoint].x == totalBounds.Max.x)
                        {
                            phase = 2;
                        }

                        break;
                    //On right, moving down.
                    case 2:
                        // 1. Get the next clockwise point from where we currently are.
                        nextPoint = currentPoint + 1;
                        if (nextPoint >= polys[currentPoly].Length)
                        {
                            nextPoint -= polys[currentPoly].Length;
                        }

                        // 2. Check its y value against the other polygons' Extents.yMax.
                        int topBottomMost = -1;
                        float tbm = float.MaxValue;
                        bestAngle = 360;
                        bestP = -1;

                        for (int i = polys.Length - 1; i >= 0; i--)
                        {
                            if (i != currentPoly)
                            {


                                if (polys[currentPoly][nextPoint].x <= polys[i].Extents.Max.x &&
                                    polys[i].Extents.Min.y <= polys[currentPoly][currentPoint].y)
                                {

                                    if (polys[i].Extents.Min.y > tbm)
                                    {
                                        topBottomMost = i;
                                        tbm = polys[i].Extents.Min.y;
                                        for (int j = polys[i].Length - 1; j >= 0; j--)
                                        {
                                            float ang = Float2.Angle(polys[currentPoly][currentPoint], polys[i][j]);
                                            if (ang < bestAngle) { bestAngle = ang; bestP = j; }
                                        }
                                    }
                                    else if (polys[i].Extents.Min.y == tbm)
                                    {
                                        for (int j = polys[i].Length - 1; j >= 0; j--)
                                        {
                                            float ang = Float2.Angle(polys[currentPoly][currentPoint], polys[i][j]);
                                            if (ang < bestAngle)
                                            {
                                                topBottomMost = i;
                                                bestAngle = ang; bestP = j;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (topBottomMost >= 0)
                        {
                            perimeterPoints.Add(polys[currentPoly][currentPoint]);
                            currentPoly = topBottomMost;
                            currentPoint = bestP;
                        }
                        else
                        {
                            //  - If no other poly's met our criteria, we accept this next point.
                            perimeterPoints.Add(polys[currentPoly][currentPoint]);
                            currentPoint = nextPoint;
                        }
                        if (polys[currentPoly][currentPoint].y == totalBounds.Min.y)
                        {
                            phase = 3;
                        }
                        break;
                    //On bottom, moving left.
                    case 3:
                        // 1. Get the next clockwise point from where we currently are.
                        nextPoint = currentPoint + 1;
                        if (nextPoint >= polys[currentPoly].Length)
                        {
                            nextPoint -= polys[currentPoly].Length;
                        }

                        // 2. Check its y value against the other polygons' Extents.yMax.
                        int rightLeftMost = -1;
                        float rlm = float.MaxValue;

                        bestP = -1;
                        bestAngle = 360;
                        for (int i = polys.Length - 1; i >= 0; i--)
                        {
                            if (i != currentPoly)
                            {

                                //  - If we have other polygons with a greater yMax, 
                                //    we take the left-most of them that's also equal to right of our current point. (NOT next point)
                                //    A. We go through the points of this other polygon. 
                                //    B. We find the angles between our current point and the points in our new poly and find the 
                                //    smallest angle, the smallest righthand turn we can make, and we call it next point.
                                if (polys[currentPoly][nextPoint].y >= polys[i].Extents.Min.y &&
                                    polys[i].Extents.Max.x <= polys[currentPoly][currentPoint].x)
                                {

                                    if (polys[i].Extents.Max.x > rlm)
                                    {
                                        rightLeftMost = i;
                                        rlm = polys[i].Extents.Max.x;
                                        for (int j = polys[i].Length - 1; j >= 0; j--)
                                        {
                                            float ang = Float2.Angle(polys[currentPoly][currentPoint], polys[i][j]);
                                            if (ang < bestAngle) { bestAngle = ang; bestP = j; }
                                        }
                                    }
                                    else if (polys[i].Extents.Max.x == rlm)
                                    {
                                        for (int j = polys[i].Length - 1; j >= 0; j--)
                                        {
                                            float ang = Float2.Angle(polys[currentPoly][currentPoint], polys[i][j]);
                                            if (ang < bestAngle)
                                            {
                                                rightLeftMost = i;
                                                bestAngle = ang; bestP = j;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (rightLeftMost >= 0)
                        {
                            perimeterPoints.Add(polys[currentPoly][currentPoint]);
                            currentPoly = rightLeftMost;
                            currentPoint = bestP;
                        }
                        else
                        {
                            //  - If no other poly's met our criteria, we accept this next point.
                            perimeterPoints.Add(polys[currentPoly][currentPoint]);
                            currentPoint = nextPoint;
                        }
                        if (polys[currentPoly][currentPoint].x == totalBounds.Min.x)
                        {
                            phase = 3;
                        }
                        break;
                    //On left, moving up.
                    case 4:
                        // 1. Get the next clockwise point from where we currently are.
                        nextPoint = currentPoint + 1;
                        if (nextPoint >= polys[currentPoly].Length)
                        {
                            nextPoint -= polys[currentPoly].Length;
                        }

                        // 2. Check its y value against the other polygons' Extents.yMax.
                        int bottomTopMost = -1;
                        float btm = float.MaxValue;
                        bestAngle = 360;
                        bestP = -1;

                        for (int i = polys.Length - 1; i >= 0; i--)
                        {
                            if (i != currentPoly)
                            {


                                if (polys[currentPoly][nextPoint].x >= polys[i].Extents.Min.x &&
                                    polys[i].Extents.Max.y >= polys[currentPoly][currentPoint].y)
                                {

                                    if (polys[i].Extents.Max.y < btm)
                                    {
                                        bottomTopMost = i;
                                        btm = polys[i].Extents.Max.y;
                                        for (int j = polys[i].Length - 1; j >= 0; j--)
                                        {
                                            float ang = Float2.Angle(polys[currentPoly][currentPoint], polys[i][j]);
                                            if (ang < bestAngle) { bestAngle = ang; bestP = j; }
                                        }
                                    }
                                    else if (polys[i].Extents.Max.y == btm)
                                    {
                                        for (int j = polys[i].Length - 1; j >= 0; j--)
                                        {
                                            float ang = Float2.Angle(polys[currentPoly][currentPoint], polys[i][j]);
                                            if (ang < bestAngle)
                                            {
                                                bottomTopMost = i;
                                                bestAngle = ang; bestP = j;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (bottomTopMost >= 0)
                        {
                            perimeterPoints.Add(polys[currentPoly][currentPoint]);
                            currentPoly = bottomTopMost;
                            currentPoint = bestP;
                        }
                        else
                        {
                            //  - If no other poly's met our criteria, we accept this next point.
                            perimeterPoints.Add(polys[currentPoly][currentPoint]);
                            currentPoint = nextPoint;
                        }
                        break;
                }
            }

            return perimeterPoints.ToArray();
        }

        #endregion

        #region Spline3 Extension Methods

        public static Float3[] GetPoints(this Point3[] points)
        {
            Float3[] p = new Float3[points.Length];
            for (int i = points.Length-1; i >= 0; i--)
            {
                p[i] = points[i].point;
            }
            return p;
        }

        public static Float3[] GetBackHandles(this Point3[] points)
        {
            Float3[] p = new Float3[points.Length];
            for (int i = points.Length-1; i >= 0; i--)
            {
                p[i] = points[i].backHandle;
            }
            return p;
        }

        public static Float3[] GetForeHandles(this Point3[] points)
        {
            Float3[] p = new Float3[points.Length];
            for (int i = points.Length-1; i >= 0; i--)
            {
                p[i] = points[i].foreHandle;
            }
            return p;
        }

        public static Bounds3 GetExtents(this Spline3[] polys)
        {
            Bounds3 r = polys[0].Extents;
            for (int i = 1; i < polys.Length; i++)
            {
                r.Encapsulate(polys[i].Extents);
            }
            return r;
        }

        public static Float3 Minimum(this Spline3[] group)
        {
            Float3 minimum = group[group.Length - 1].Min;
            for (int i = group.Length - 3; i >= 0; i--)
            {

                if (group[i].Min.x < minimum.x)
                {
                    minimum.x = group[i].Min.x;
                }
                if (group[i].Min.y < minimum.y)
                {
                    minimum.y = group[i].Min.y;
                }
                if (group[i].Min.z < minimum.z)
                {
                    minimum.z = group[i].Min.z;
                }
            }
            return minimum;
        }

        public static Float3 SmoothMinimum(this Spline3[] group)
        {
            Float3 minimum = group[group.Length - 1].SmoothMin;
            for (int i = group.Length - 3; i >= 0; i--)
            {

                if (group[i].SmoothMin.x < minimum.x)
                {
                    minimum.x = group[i].SmoothMin.x;
                }
                if (group[i].SmoothMin.y < minimum.y)
                {
                    minimum.y = group[i].SmoothMin.y;
                }
                if (group[i].SmoothMin.z < minimum.z)
                {
                    minimum.z = group[i].SmoothMin.z;
                }
            }
            return minimum;
        }

        public static Float3 Maximum(this Spline3[] group)
        {
            Float3 maximum = group[group.Length - 1].Max;
            for (int i = group.Length - 2; i >= 0; i--)
            {

                if (group[i].Max.x < maximum.x)
                {
                    maximum.x = group[i].Max.x;
                }
                if (group[i].Max.y < maximum.y)
                {
                    maximum.y = group[i].Max.y;
                }
                if (group[i].Max.z < maximum.z)
                {
                    maximum.z = group[i].Max.z;
                }
            }
            return maximum;
        }

        public static Float3 SmoothMaximum(this Spline3[] group)
        {
            Float3 maximum = group[group.Length - 1].SmoothMax;
            for (int i = group.Length - 2; i >= 0; i--)
            {

                if (group[i].SmoothMax.x < maximum.x)
                {
                    maximum.x = group[i].SmoothMax.x;
                }
                if (group[i].SmoothMax.y < maximum.y)
                {
                    maximum.y = group[i].SmoothMax.y;
                }
                if (group[i].SmoothMax.z < maximum.z)
                {
                    maximum.z = group[i].SmoothMax.z;
                }
            }
            return maximum;
        }

        #endregion

        #region Spline4 Extension Methods
        #endregion
    }

    public abstract class SplineBase
    {
        #region Variables

        public string name;
        public CurveType defaultCurveType = CurveType.Linear;
        public float defaultWeight = 1f, defaultDropValue, weightedPercent, totalWeight, totalLength;

        public bool splineDirty = true, anchorsDirty = true;
        public int[] newIDS;

        public int minPoint, minSmoothPoint, maxPoint, maxSmoothPoint;

        public SplineSection[] sections = new SplineSection[0];

        public SplineSection GetSection(int id)
        {
            if (id < 0 || id >= sections.Length) { return new SplineSection(); }

            return sections[id];
        }

        public bool wrap;

        protected bool initialized = false;

        public int currentSection = -1;
        #endregion

        #region Properties
        public abstract int Length { get; }

        public abstract bool Wrap { get; set; }

        public abstract int Sections { get; set; }
        #endregion

        #region Methods

        public float GetPercentAtPoint(int p, bool weighted = true)
        {

            float per = 0;
            

            if (weighted)
            {
                for (int i = 0; i < p; i++)
                {
                    per += sections[i].weight;
                }
                per /= totalWeight;
            }
            else
            {
                for (int i = 0; i < p; i++)
                {
                    per += sections[i].length;
                }
                per /= totalLength;
            }
            return per;
        }

        public int GetSectionAtPercent(float p, bool weighted = true)
        {
            int i = 0;

            if (wrap)
            {
                
                if (p > 1) { i = (int)Math.Floor(p); p -= i; }
                else if (p < 0) { i = (int)Math.Abs(Math.Floor(p)); p += i; }

            }


            if (p >= 0 && p <= 1)
            {
                if (weighted)
                {
                    float currentWeight = 0f, c;
                    float percentAsWeight = p * totalWeight;

                    for (i = 0; i < sections.Length; i++)
                    {
                        c = currentWeight + sections[i].weight;
                        if (currentWeight <= percentAsWeight && c > percentAsWeight)
                        {

                            weightedPercent = (percentAsWeight - currentWeight) / sections[i].weight;
                            return i;
                        }
                        else
                        {
                            currentWeight = c;
                        }
                    }
                }
                else
                {
                    float currentDistance = 0f, c;
                    float percentAsDistance = p * totalLength;

                    for (i = 0; i < sections.Length; i++)
                    {
                        c = currentDistance + sections[i].length;
                        if (currentDistance <= percentAsDistance && c > percentAsDistance)
                        {

                            weightedPercent = (percentAsDistance - currentDistance) / sections[i].length;
                            return i;
                        }
                        else
                        {
                            currentDistance = c;
                        }
                    }
                }
            }
            else
            {
                weightedPercent = 0;
                return 0;
            }
            return currentSection;
        }

        public float GetWeightToPoint(int p)
        {
            float weightToPoint = 0;
            for (int i = 0; i < p; i++)
            {
                weightToPoint += sections[i].weight;
            }
            return weightToPoint;
        }

        public float GetTotalWeight()
        {
            totalWeight = 0;
            for (int i = Sections - 1; i >= 0; i--)
            {
                totalWeight += sections[i].weight;
            }
            return totalWeight;
        }

        public abstract float GetTotalSplineLength(float samplePercent, bool resetHeldValue = false);

        public abstract void RemovePoint(int id);

        public abstract void RemovePoints(int[] ids);

        public abstract void Scale(float scalar);

        public abstract void Normalize();

        public abstract float Distance(float p1, float p2);

        public abstract float Distance(float p1, float p2, int samples);

        public abstract void SetCapPoints();

        public abstract void GetControlPoints();

        public abstract void ResamplePoints(int samples);

        public abstract int GetSectionID(int smoothID);
        #endregion

    }


    [System.Serializable]
    public class Spline1 : SplineBase
    {

        #region Variables

        public Point1[] points = new Point1[0];

        public Float[] smoothedPoints;

        public Bounds extents;
        public bool extentsDirty = true;

        public Float startCap, endCap;

        Float min, max, smoothMin, smoothMax,
              a, b, c, d;

        #endregion

        #region Properties

        public Float this[int index]
        {
            get { return points[index].point; }
            set
            {
                anchorsDirty = true;
                if (wrap && (index == 0 || index == Length))
                {
                    points[0].point = points[Length].point = value;
                }
                else
                {
                    points[index].point = value;
                }
                //Debug.Log("Setting slot " + index + " to " + value);
            }
        }

        public override int Length
        {
            get { if (wrap) { return points.Length - 1; } else { return points.Length; } }
        }

        public override bool Wrap
        {
            get { return wrap; }
            set
            {
                if (value != wrap)
                {
                    if (value)
                    {
                        if ((points[0].point != points[points.Length - 1].point))
                        {
                            System.Array.Resize<Point1>(ref points, points.Length + 1);
                            points[points.Length - 1] = points[0];

                        }
                    }
                    else
                    {
                        if ((points[0].point == points[points.Length - 1].point))
                        {
                            System.Array.Resize(ref points, points.Length - 1);
                        }
                    }
                    Sections = points.Length - 1;
                    anchorsDirty = true;
                    splineDirty = true;
                    SetCapPoints();
                    GetTotalWeight();
                    GetTotalSplineLength(1);
                }
                wrap = value;
            }
        }

        public int MinimumPoint { get { if (extentsDirty) { Bounds b = Extents; } return minPoint; } }

        public int MinimumSmoothPoint { get { if (extentsDirty) { Bounds b = Extents; } return minSmoothPoint; } }

        public int MaximumPoint { get { if (extentsDirty) { Bounds b = Extents; } return maxPoint; } }

        public int MaximumSmoothPoint { get { if (extentsDirty) { Bounds b = Extents; } return maxSmoothPoint; } }

        public Float Min { get { return min; } }

        public Float Max { get { return max; } }

        public Float SmoothMin { get { return smoothMin; } }

        public Float SmoothMax { get { return smoothMax; } }

        public Float[] AnchorPoints
        {
            get
            {
                Float[] nP = new Float[points.Length];
                for (int i = nP.Length - 1; i >= 0; i--)
                {
                    nP[i] = points[i].point;
                }
                return nP;
            }
        }

        public Bounds Extents
        {
            get
            {
                if (extentsDirty)
                {
                    extentsDirty = false;
                    extents = new Bounds(points[Length - 1].point, 0f);
                    minPoint = maxPoint = minSmoothPoint = maxSmoothPoint = Length - 1;
                    

                    Float[] sP = SmoothedPoints;

                    int currentSegment = newIDS[Length - 1];
                    for (int i = Length - 2; i >= 0; i--)
                    {

                        if (points[i].point < points[minPoint].point)
                        {
                            minPoint = i;
                            min = points[i].point;
                        }

                        if (points[i].point > points[maxPoint].point)
                        {
                            maxPoint = i;
                            max = points[i].point;
                        }

                        int nextSegment = newIDS[i];
                        for (int j = currentSegment - 1; j >= nextSegment; j--)
                        {

                            if (sP[j].x < min.x)
                            {
                                min.x = sP[j].x;
                                minSmoothPoint = j;
                                smoothMin = sP[j];
                            }

                            if (sP[j].x > max.x)
                            {
                                max.x = sP[j].x;
                                maxSmoothPoint = j;
                                smoothMax = sP[j];
                            }
                        }
                        currentSegment = nextSegment;
                    }
                }
                return extents;
            }
        }

        public override int Sections
        {
            get { return sections.Length; }
            set
            {
                if (value != sections.Length)
                {
                    int newLength = value;
                    if (newLength > points.Length - 1)
                    {
                        newLength = points.Length - 1;
                    }
                    if (newLength > 0)
                    {
                        SplineSection[] newSec = new SplineSection[newLength];

                        totalWeight = 0;

                        int lesser = sections.Length;
                        if (newLength < lesser)
                        {
                            lesser = newLength;
                        }
                        for (int i = lesser - 1; i >= 0; i--)
                        {
                            newSec[i] = sections[i];
                        }

                        if (newLength > lesser)
                        {
                            for (int i = lesser; i < newLength; i++)
                            {
                                newSec[i].curveType = defaultCurveType;
                                newSec[i].weight = defaultWeight;
                                totalWeight += defaultWeight;
                            }
                        }
                        sections = newSec;
                    }
                }
            }
        }

        public Float[] SmoothedPoints
        {
            get
            {
                if (anchorsDirty)
                {
                    SetCapPoints();
                    smoothedPoints = GetSmoothedPoints(out newIDS);
                    anchorsDirty = false;
                }
                return smoothedPoints;
            }
        }

        #endregion
        
        #region Methods
        
        #region Constructors

        public Spline1()
        {

            defaultCurveType = CurveType.Linear;
            defaultWeight = 1f;
            defaultDropValue = 0;

            points = new Point1[0];

            sections = new SplineSection[0];
            Sections = 0;
            totalLength = 0;
            anchorsDirty = splineDirty = true;
        }

        public Spline1(int p, CurveType dst = CurveType.Linear)
        {
            if (p <= 1) { p = 2; }

            points = new Point1[p];
            sections = new SplineSection[p - 1];

            defaultCurveType = dst;
            defaultWeight = 1f;
            defaultDropValue = 0;

            float inv = 1f / p;
            points[p - 1].point = Float.one;
            totalWeight = 0;
            for (int i = p - 2; i >= 0; i--)
            {
                points[i].point.x = inv * i;
                sections[i].weight = defaultWeight;
                totalWeight += defaultWeight;
                sections[i].curveType = defaultCurveType;

            }

            Sections = p - 1;
            totalLength = GetTotalSplineLength(0.1f);
            smoothedPoints = GetSmoothedPoints(out newIDS);
            anchorsDirty = splineDirty = true;
        }

        public Spline1(float[] p, CurveType curveType = CurveType.Linear, bool doWrap = false)
        {
            defaultCurveType = curveType;
            defaultWeight = 1f;
            defaultDropValue = 0;
            wrap = doWrap;
            if (wrap)
            {
                points = new Point1[p.Length + 1];
                for (int i = p.Length - 1; i >= 0; i--)
                {
                    points[i] = new Point1();
                    points[i].point = p[i];
                }
                points[p.Length] = points[0];
                Wrap = true;
                Sections = p.Length;
            }
            else
            {
                points = new Point1[p.Length];
                for (int i = p.Length - 1; i >= 0; i--)
                {
                    points[i] = new Point1();
                    points[i].point = p[i];
                }
                Sections = p.Length - 1;
            }


            totalLength = 0;

            anchorsDirty = extentsDirty = true;
        }

        public Spline1(Spline1 spline, int p = -1, bool weighted = true, bool guaranteeResample = false)
        {
            if (p < 0) { p = spline.Length; }
            if (p < 2) { p = 2; }
            spline.GetTotalWeight();
            spline.GetTotalSplineLength(0.01f);
            if (p == spline.Length && !guaranteeResample)
            {

                name = spline.name;
                spline.GetAnchorPoints(out points);
                sections = spline.sections;
                totalWeight = spline.totalWeight;

                wrap = spline.wrap;
                minPoint = spline.minPoint;
                maxPoint = spline.maxPoint;
                min = spline.min;
                max = spline.max;
            }
            else
            {
                points = new Point1[p];

                sections = new SplineSection[p - 1];

                wrap = spline.wrap;
                float per = 1f / p;
                int s = -1;
                if (p > 2)
                {

                    for (int i = 0; i < p - 1; i++)
                    {
                        s = spline.GetSectionAtPercent(per * i, weighted);

                        points[i].point = spline.GetPoint(per * i, weighted);
                        if (!weighted)
                        {
                            sections[i].weight = 1f;
                        }
                        else
                        {
                            sections[i].weight = per * spline.sections[s].weight;
                        }
                        totalWeight += sections[i].weight;
                        sections[i].curveType = spline.sections[s].curveType;
                    }
                    points[p - 1] = spline.points[spline.Length - 1];
                }
                else
                {
                    points[0].point = spline.points[0].point;
                    sections[0].weight = spline.sections[0].weight;

                    sections[0].curveType = spline.sections[0].curveType;
                    points[1].point = spline.points[spline.Length - 1].point;
                }
                if (wrap && (points[0].point != points[points.Length - 1].point))
                {
                    System.Array.Resize<Point1>(ref points, points.Length + 1);
                    points[points.Length - 1] = points[0];
                }
            }
            totalLength = GetTotalSplineLength(0.01f);
            anchorsDirty = splineDirty = true;
        }

        public static Spline1 Linear()
        {
            Spline1 s = new Spline1(2, CurveType.Linear);
            s[0] = 0; s[1] = 1;
            return s;
        }

        #endregion

        public override void Scale(float scalar)
        {
            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i].point *= scalar;
                if (points[i].useForeHandle) points[i].foreHandle *= scalar;
                if (points[i].useBackHandle) points[i].backHandle *= scalar;
            }
            totalLength = GetTotalSplineLength(0.1f);
            anchorsDirty = true;
        }

        public override void Normalize()
        {
            Bounds minmax = Extents;
            Float offset = smoothMin;
            Float scalar = smoothMax - smoothMin;

            offset *= -1f;
            if (scalar == 0) { scalar = 1f; }
            else
            {
                scalar = 1f / scalar;
            }

            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i].point += offset;
                points[i].point *= scalar;
                if (points[i].useForeHandle) points[i].foreHandle *= scalar;
                if (points[i].useBackHandle) points[i].backHandle *= scalar;
            }
            totalLength = GetTotalSplineLength(0.1f);
            anchorsDirty = true;
        }

        public void Offset(float offset)
        {
            for (int i = Length - 1; i >= 0; i--)
            {
                this[i] += offset;
            }
            anchorsDirty = true;
            splineDirty = true;
        }

        public override float Distance(float p1, float p2)
        {
            return Distance(p1, p2, 20);
        }

        public override float Distance(float p1, float p2, int samples)
        {
            float distance = 0f;
            float p = p2 - p1 / samples;
            float cP = p1;
            float currentPoint = GetPoint(p1);
            float prevPoint = currentPoint;

            for (int i = samples - 1; i >= 0; i--)
            {
                cP += p;
                currentPoint = GetPoint(cP);
                distance += Math.Abs(currentPoint - prevPoint);
                prevPoint = currentPoint;
            }
            return distance;
        }


        public void SetAnchorPoints(Point1[] newPoints)
        {
            points = newPoints;
            if (wrap && points[0].point != points[points.Length - 1].point)
            {
                System.Array.Resize(ref points, points.Length + 1);
                points[points.Length - 1] = points[0];
            }

            Sections = points.Length - 1;
            totalLength = GetTotalSplineLength(0.1f);
            anchorsDirty = splineDirty = true;
        }

        public void GetAnchorPoints(out Point1[] p)
        {
            int l = points.Length;
            p = new Point1[l];
            for (int i = points.Length - 1; i >= 0; i--)
            {
                p[i] = points[i];
            }
        }


        public void AddPoints(float[] p, CurveType[] s, float[] t, int id = -1)
        {

            for (int i = 0; i < p.Length; i++)
            {
                int nid = id;
                if (nid != -1)
                {
                    nid += i;
                }
                AddPoint(p[i], nid, defaultCurveType, defaultWeight);
            }
        }

        public void AddPoint(float p, int id = -1, CurveType s = CurveType.None, float t = -1f)
        {

            if (s == CurveType.None)
            {

                s = defaultCurveType;
            }
            if (t < 0)
            {
                t = defaultWeight;
            }


            int npc = points.Length;
            int fillerSlots = 0;
            if (id > npc) { fillerSlots = id - npc; }

            totalWeight = 0;
            List<Point1> newPoints = new List<Point1>(points);
            List<SplineSection> newSections = new List<SplineSection>(sections);

            if (id < 0)
            {
                Point1 newP = new Point1();
                SplineSection newS = new SplineSection { curveType = s, weight = t };
                newP.point = p;
                newPoints.Add(newP);
                newSections.Add(newS);
                totalWeight += t;

            }
            else if (id < points.Length)
            {
                Point1 newP = new Point1();
                SplineSection newS = new SplineSection { curveType = s, weight = t };
                newP.point = p;
                newPoints.Insert(id, newP);
                newSections.Insert(id, newS);
                totalWeight += t;

            }
            else
            {
                for (int i = 0; i < fillerSlots; i++)
                {
                    newPoints.Add(new Point1());
                    newSections.Add(new SplineSection { curveType = defaultCurveType, weight = defaultWeight });
                    totalWeight += defaultWeight;

                }

                Point1 p1 = new Point1();
                SplineSection newS = new SplineSection { curveType = s, weight = t };

                p1.point = p;
                newPoints.Add(p1);
                newSections.Add(newS);
                totalWeight += t;

            }

            points = newPoints.ToArray();
            sections = newSections.ToArray();

            anchorsDirty = true;
            splineDirty = true;
            SetCapPoints();
            GetTotalWeight();
            GetTotalSplineLength(1);
        }

        public void InsertPoint(float point, float percent)
        {
            int sec = GetSectionAtPercent(percent);
            if (sec < 0 || sec >= points.Length - 1) { return; }

            List<Point1> newPoints = new List<Point1>(points);
            List<SplineSection> newSections = new List<SplineSection>(sections);

            Point1 newPoint = new Point1() { point = point };

            float sectionPercent = GetPercentAtPoint(sec), nextPercent = GetPercentAtPoint(sec + 1);
            float percentDiff = percent - sectionPercent;

            sectionPercent = nextPercent - sectionPercent;
            percentDiff /= sectionPercent;

            SplineSection oldSection = new SplineSection()
            {
                curveType = sections[sec].curveType,
                length = sections[sec].length * percentDiff,
                weight = sections[sec].weight * percentDiff
            };
            SplineSection newSection = new SplineSection()
            {
                curveType = sections[sec].curveType,
                length = sections[sec].length - oldSection.length,
                weight = sections[sec].weight - oldSection.weight
            };

            /*
            string insertBreakdown = "Inserting point at percent: " + percent + System.Environment.NewLine;

            insertBreakdown += "Inserting into section " + sec + ". Original Length: " + sections[sec].length +
                "  Original Weight: " + sections[sec].weight + System.Environment.NewLine;

            insertBreakdown += "Old Section - Length: " + oldSection.length + "  Weight: " + oldSection.weight +
                System.Environment.NewLine + "New Section = Length: " + newSection.length + "  Weight: " + newSection.weight;
            Debug.Log(insertBreakdown);
            */

            if (points[sec].useForeHandle) { }
            if (points[sec + 1].useBackHandle) { }

            newSections[sec] = oldSection;

            newPoints.Insert(sec + 1, newPoint);
            newSections.Insert(sec + 1, newSection);

            points = newPoints.ToArray();
            sections = newSections.ToArray();
            anchorsDirty = splineDirty = true;
            SetCapPoints();

        }

        public override void RemovePoint(int id)
        {

            if (id >= 0 || id < points.Length)
            {
                List<Point1> newPoints = new List<Point1>(points);


                newPoints.RemoveAt(id);
                points = newPoints.ToArray();
                Sections = points.Length - 1;

            }
        }

        public override void RemovePoints(int[] ids)
        {
            List<Point1> newPoints = new List<Point1>(points);
            List<SplineSection> newsections = new List<SplineSection>(sections);

            for (int i = Length - 1; i >= 0; i--)
            {

                for (int j = 0; j < ids.Length; j++)
                {
                    if (i == ids[j])
                    {
                        newPoints.RemoveAt(ids[j]); newsections.RemoveAt(ids[j]);
                    }
                }

            }

            points = newPoints.ToArray();

            sections = newsections.ToArray();
        }

        public override void SetCapPoints()
        {
            //First, how many points do we actually have?
            int l = points.Length;

            //We need at least two points.
            if (l > 1)
            {
                //Our starting control point is the difference between point 0 and point 1 subtracted from point 0.
                Float pointDiff = points[1].point; pointDiff -= points[0].point;
                startCap = points[0].point; startCap -= pointDiff;

                if (l >= 3)
                {
                    //We check to see if this is a loop - if the first and last official points are the same.
                    // If so, we set our control points to form that perfect, continuous loop.
                    if (points[0].point == points[l - 1].point)
                    {
                        startCap = points[l - 2].point;
                        endCap = points[1].point;
                    }
                    //If not, we create our two control points. Each is just a straight line from our second set of points, going inward, through our first 
                    //set of points.	
                    else
                    {
                        pointDiff = points[l - 1].point;
                        pointDiff -= points[l - 2].point;
                        endCap = points[l - 1].point; endCap += pointDiff;
                    }
                }
                else
                {
                    endCap = points[1].point; endCap += pointDiff;
                }
            }
        }

        public Float GetPoint(float percent, bool weighted = true)
        {

            float currentPoint;
            if (points.Length > 0)
            {
                if (points.Length > 1)
                {
                    if (percent <= 0)
                    {
                        currentPoint = points[0].point;
                    }
                    else if (percent >= 1)
                    {
                        currentPoint = points[points.Length - 1].point;
                    }
                    else
                    {

                        int cS = GetSectionAtPercent(percent, weighted);

                        if (cS != currentSection)
                        {
                            currentSection = cS;
                            GetControlPoints();
                        }

                        float point = 0;

                        switch (sections[cS].curveType)
                        {
                            //Catmull Rom
                            case CurveType.CatmullRom:
                                point = Splines.CatmullRom(weightedPercent, a, b, c, d);
                                break;

                            //Linear
                            case CurveType.Linear:
                                point = Float.Lerp(points[cS].point, points[cS + 1].point, weightedPercent);
                                break;

                            //CubicBezier
                            case CurveType.CubicBezier:
                                point = Splines.CubicBezier(weightedPercent, a, b, c, d);
                                break;

                            //Step
                            case CurveType.Step:
                                point = points[cS].point;
                                break;
                        }
                        currentPoint = point;
                    }
                }
                else
                {
                    currentPoint = points[0].point;
                }
            }
            else
            {
                currentPoint = 0;
            }
            return currentPoint;
        }

        public override void GetControlPoints()
        {
            int l = sections.Length;
            if (currentSection == 0 && l > 1)
            {
                a = startCap;
                b = points[0].point;
                c = points[1].point;
                d = points[2].point;
            }
            else if (currentSection == 0)
            {
                a = startCap;
                b = points[0].point;
                c = points[1].point;
                d = endCap;
            }
            else if (currentSection == l - 1)
            {
                a = points[l - 2].point;
                b = points[l - 1].point;
                c = points[l].point;
                d = endCap;
            }
            else if (currentSection > 0)
            {
                a = points[currentSection - 1].point;
                b = points[currentSection].point;
                c = points[currentSection + 1].point;
                d = points[currentSection + 2].point;
            }

            if (points[currentSection].useBackHandle)
            {
                a = points[currentSection].point + points[currentSection].backHandle;
            }
            if (points[currentSection + 1].useForeHandle)
            {
                d = points[currentSection + 1].point + points[currentSection + 1].foreHandle;
            }
        }

        public int GetClosestAnchorPoint(float p, PointSearchType searchType = PointSearchType.Any)
        {
            float lowestDist = float.PositiveInfinity;
            int id = -1;
            switch (searchType)
            {
                case PointSearchType.Any:
                    for (int i = Length - 1; i >= 0; i--)
                    {

                        float d = Math.Abs(this[i] - p);
                        if (d < lowestDist)
                        {
                            id = i;
                            lowestDist = d;
                        }
                    }
                    break;

                case PointSearchType.Before:
                    for (int i = Length - 1; i >= 0; i--)
                    {
                        if (this[i] > p) { continue; }
                        float d = Math.Abs(this[i] - p);
                        if (d < lowestDist)
                        {
                            id = i;
                            lowestDist = d;
                        }
                    }
                    break;

                case PointSearchType.After:
                    for (int i = Length - 1; i >= 0; i--)
                    {
                        if (this[i] < p) { continue; }
                        float d = Math.Abs(this[i] - p);
                        if (d < lowestDist)
                        {
                            id = i;
                            lowestDist = d;
                        }
                    }
                    break;
            }

            return id;
        }

        public override float GetTotalSplineLength(float samplePercent, bool resetHeldValue = false)
        {

            float sLength = 0f;
            if (sections.Length > 0)
            {
                float secPer = 0;
                float sp = 0;
                for (int j = 0; j < sections.Length; j++)
                {
                    sections[j].length = 0;
                    if (sections[j].curveType == CurveType.Linear)
                    {
                        sections[j].length = points[j + 1].point - points[j].point;
                        sLength += sections[j].length;
                    }
                    else if (sections[j].curveType == CurveType.CatmullRom || sections[j].curveType == CurveType.CubicBezier)
                    {

                        float a = points[j].point, b;
                        secPer += sections[j].weight;
                        float isp = secPer / totalWeight;

                        for (float i = 0; i <= 1; i += samplePercent)
                        //for (float i = samplePercent; i <= 1; i += samplePercent)
                        {
                            b = GetPoint(Float.Lerp(sp, isp, i));
                            float d = b - a;
                            a = b;
                            sections[j].length += d;
                            sLength += d;
                        }
                        sp = isp;
                    }
                }
            }


            if (resetHeldValue) { totalLength = sLength; }
            return sLength;
        }

        public Float AveragePoints(bool useSmoothed = true)
        {

            Float pointAv = 0f;

            if (useSmoothed)
            {
                Float[] pts = SmoothedPoints;
                for (int i = 0; i < pts.Length; i++)
                {
                    pointAv += pts[i];
                }
                pointAv /= pts.Length;
            }
            else
            {
                for (int i = 0; i < points.Length; i++)
                {
                    pointAv += points[i].point;

                }
                pointAv /= points.Length;
            }
            return pointAv;
        }

        public override void ResamplePoints(int samples)
        {
            if (samples > 2)
            {

                float perc = 1f / (samples - 1);

                Point1[] newPoints = new Point1[samples];
                SplineSection[] secs = new SplineSection[samples - 1];

                newPoints[0] = points[0];
                secs[0] = sections[0];

                int s = 0;
                for (int i = 1; i < points.Length; i++)
                {
                    float ip = GetPercentAtPoint(i), sp = s * perc;
                    while (sp < ip)
                    {
                        s++;
                        newPoints[s] = points[i];
                        newPoints[s].point = GetPoint(sp);
                        if (i < points.Length - 1) { secs[s] = sections[i]; }
                        sp = s * perc;
                    }
                    if (s == samples) { break; }
                }

                points = newPoints;
                sections = secs;
            }
        }

        public Float[] GetSmoothedPoints(out int[] newIDS, int maxSubdivisions = 10, float subDivideAngle = 15)
        {
            if (maxSubdivisions < 3) { maxSubdivisions = 3; }
            

            List<Float> newPoints = new List<Float>();
            List<int> nIDS = new List<int>();
            for (int i = 0; i < points.Length - 1; i++)
            {
                nIDS.Add(newPoints.Count);
                newPoints.Add(points[i].point);

                switch (sections[i].curveType)
                {
                    case CurveType.CatmullRom:
                    case CurveType.CubicBezier:
                        currentSection = -1;
                        float pa = GetPercentAtPoint(i), pb = GetPercentAtPoint(i + 1);

                        float diff = pb - pa;
                        float per = diff / maxSubdivisions;

                        for (int j = 1; j < maxSubdivisions; j++)
                        {
                            newPoints.Add(GetPoint(pa + (per * j)));
                        }

                        break;
                }
            }
            nIDS.Add(newPoints.Count);
            newPoints.Add(points[points.Length - 1].point);

            newIDS = nIDS.ToArray();
            return newPoints.ToArray();
        }

        public override int GetSectionID(int smoothID)
        {

            int currentSec = newIDS[points.Length - 1];
            if (smoothID <= currentSec)
            {
                for (int i = points.Length - 2; i >= 0; i--)
                {
                    if (smoothID < currentSec)
                    {
                        return i;
                    }
                    currentSec = newIDS[i];
                }
                return points.Length - 1;
            }
            return -1;
        }

        public Spline1 SplitSpline(int id, bool includeCurrent)
        {

            if (points.Length > id)
            {
                List<Point1> newPoints = new List<Point1>(points);
                List<SplineSection> newsections = new List<SplineSection>(sections);


                int diff = Length - id;
                List<Point1> splitPoints = newPoints.GetRange(id, diff);
                List<SplineSection> splitSections = newsections.GetRange(id, diff);



                points = newPoints.GetRange(0, id - 1).ToArray();
                sections = newsections.GetRange(0, id - 1).ToArray();

                Spline1 leftOvers = new Spline1();
                leftOvers.SetAnchorPoints(splitPoints.ToArray());


                return leftOvers;
            }
            else
            {
                return new Spline1();
            }
        }

        public float[] GetPercentsAtValue(float value, int samples)
        {
            List<float> percents = new List<float>();

            for (int i = 0; i < points.Length - 1; i++)
            {
                float a = this[i], b = -1, c = this[i];

                if (value > a && value < c)
                {
                    float aPer = GetPercentAtPoint(i), bPer = -1, cPer = GetPercentAtPoint(i + 1);
                    for (int j = 0; j < samples; j++)
                    {
                        bPer = (aPer + cPer) * 0.5f;
                        b = GetPoint(bPer);
                        if (value > b)
                        {
                            a = b;
                            aPer = bPer;
                        }
                        else if (value < b)
                        {
                            c = b;
                            cPer = bPer;
                        }
                        else { percents.Add(bPer); }
                    }
                    if (value > b)
                    {
                        //check if C or B is closer. Return that one.
                        float diffC = c - value;
                        float diffB = value - b;
                        if (diffC < diffB) { percents.Add(cPer); }
                        else { percents.Add(bPer); }
                    }
                    else if (value < b)
                    {
                        //check a/b
                        float diffB = b - value;
                        float diffA = value - a;
                        if (diffA < diffB) { percents.Add(aPer); }
                        else { percents.Add(bPer); }
                    }
                }
            }
            return percents.ToArray();
        }

        public Spline1 GetSubSpline(int start, int end)
        {
            List<Point1> newPoints = new List<Point1>(points);
            List<SplineSection> newsections = new List<SplineSection>(sections);

            int diff = end - start;
            int ddif = diff - 1;
            Spline1 s = new Spline1();
            if (start > end)
            {
                if (!wrap) { return null; }
                //diff = points.Length - 1 - start;
                diff = points.Length - start;
                ddif = diff - 1;
                newPoints = newPoints.GetRange(start, diff);
                newsections = newsections.GetRange(start, ddif);

                for (int i = 1; i <= end; i++)
                {
                    newPoints.Add(points[i]);
                    newsections.Add(sections[i]);
                }
            }
            else
            {
                newPoints = newPoints.GetRange(start, diff);
                newsections = newsections.GetRange(start, ddif);
            }

            s.points = newPoints.ToArray();

            s.sections = newsections.ToArray();

            if (start > 0)
            {
                s.startCap = points[start - 1].point;

            }
            if (end < points.Length - 1)
            {
                s.endCap = points[end + 1].point;

            }

            s.anchorsDirty = true;

            s.GetTotalWeight();
            s.GetTotalSplineLength(0.01f, true);
            return s;
        }

        public Spline1 GetSubSpline(float start, float end)
        {
            if (start == 0 && end == 1) { return new Spline1(this); }

            float startValue = GetPoint(start), endValue = GetPoint(end);
            Point1 startPoint = new Point1 { point = startValue }, endPoint = new Point1 { point = endValue };

            //Ok, I know this is going to need some work. But it'll do for now.
            //float bh = 0, fh = 0;
            //if (nearStart > 0) { bh = this[nearStart - 1]; }
            //if(nearEnd < points.Length - 1) { fh = this[points.Length - 1]; }

            List<Point1> newPoints = new List<Point1>();
            newPoints.Add(startPoint);
            List<SplineSection> newsections = new List<SplineSection>();
            newsections.Add(new SplineSection { curveType = CurveType.CatmullRom, weight = defaultWeight });

            Spline1 s = new Spline1();

            if (start > end)
            {
                if (!wrap) { return null; }

                s.wrap = true;

                for (int i = 0; i < points.Length - 1; i++)
                {
                    if (points[i].point > startValue)
                    {
                        newPoints.Add(points[i]);
                        newsections.Add(sections[i]);
                    }
                }
                for (int i = 0; i < points.Length - 1; i++)
                {
                    if (points[i].point < endValue)
                    {
                        newPoints.Add(points[i]);
                        newsections.Add(sections[i]);
                    }
                    else { break; }
                }
            }
            else
            {
                for (int i = 0; i < points.Length; i++)
                {
                    if (points[i].point > endValue) { break; }

                    if (points[i].point > startValue)
                    {
                        newPoints.Add(points[i]);
                        newsections.Add(sections[i]);
                    }

                }
            }

            newPoints.Add(endPoint);

            s.points = newPoints.ToArray();
            s.sections = newsections.ToArray();

            s.anchorsDirty = true;

            s.GetTotalWeight();
            s.GetTotalSplineLength(0.01f, true);
            return s;
        }

        public static Spline1 Combine(Spline1[] splines, bool trimEnds = true)
        {
            if (splines.Length == 0) { return null; }
            if (splines.Length < 2) { return splines[0]; }


            List<Point1> sPoints = new List<Point1>(splines[0].points);
            List<SplineSection> sSections = new List<SplineSection>(splines[0].sections);
            if (trimEnds)
            {
                sPoints.RemoveAt(sPoints.Count - 1);
            }
            for (int i = 0; i < splines.Length; i++)
            {
                int jStart = 0, jEnd = splines[i].points.Length; if (trimEnds) { jStart = 1; if (i != splines.Length - 1) jEnd--; }
                for (int j = jStart; j < jEnd; j++)
                {

                    sPoints.Add(splines[i].points[j]);
                    if (j < jEnd - 1) { sSections.Add(splines[i].sections[j]); }
                }
            }

            if (sPoints.Count < 2) { return null; }

            Spline1 s = new Spline1();
            s.points = sPoints.ToArray();
            s.sections = sSections.ToArray();
            s.anchorsDirty = true;
            s.GetTotalSplineLength(0.01f);
            s.GetTotalWeight();

            return s;
        }

        public static Spline1 ZeroToOne()
        {
            Spline1 zto = new Spline1(2);
            zto[0] = 0f;
            zto[1] = 1f;
            return zto;
        }

        public static Spline1 OneToZero()
        {
            Spline1 otz = new Spline1(2);
            otz[0] = 1f;
            otz[1] = 0f;
            return otz;
        }
        #endregion


    }
    
    [System.Serializable]
    public class Spline2 : SplineBase
    {
        #region Variables
        public XYToXYZ axisConversion;
        public Point2[] points = new Point2[0];

        public Bounds2 extents;
        public bool extentsDirty = true;

        public int top, smoothTop, left, smoothLeft, bottom, smoothBottom, right, smoothRight;

        public Float2[] smoothedPoints = new Float2[0];

        public Float2 startCap, endCap;

        Float2 a, b, c, d;

        Float2 min, max, smoothMin, smoothMax;

        float splineLength = -1;

        #endregion

        #region Properties
        /// <summary>
        /// The top-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Top { get { if (extentsDirty) { Bounds2 e = Extents; } return top; } }

        public int SmoothTop { get { if (extentsDirty) { Bounds2 e = Extents; } return smoothTop; } }

        /// <summary>
        /// The left-most point. If two or more points share the same min X value, the bottom-most point is used.
        /// </summary>
        public int Left { get { if (extentsDirty) { Bounds2 e = Extents; } return left; } }

        public int SmoothLeft { get { if (extentsDirty) { Bounds2 e = Extents; } return smoothLeft; } }

        /// <summary>
        /// The bottom-most point. If two or more points share the same min Y value, the right-most point is used.
        /// </summary>
        public int Bottom { get { if (extentsDirty) { Bounds2 e = Extents; } return bottom; } }

        public int SmoothBottom { get { if (extentsDirty) { Bounds2 e = Extents; } return smoothBottom; } }

        /// <summary>
        /// The right-most point. If two or more points share the same max X value, the top-most point is used.
        /// </summary>
        public int Right { get { if (extentsDirty) { Bounds2 e = Extents; } return right; } }

        public int SmoothRight { get { if (extentsDirty) { Bounds2 e = Extents; } return smoothRight; } }

        public Float2 this[int index]
        {
            get { return points[index].point; }
            set
            {
                anchorsDirty = true;
                if (wrap && (index == 0 || index == Length))
                {
                    points[0].point = points[Length].point = value;
                }
                else
                {
                    points[index].point = value;

                }
            }
        }

        public Float2[] AnchorPoints
        {
            get
            {
                Float2[] nP = new Float2[points.Length];
                for (int i = nP.Length - 1; i >= 0; i--)
                {
                    nP[i] = points[i].point;
                }
                return nP;
            }
        }

        public Bounds2 Extents
        {
            get
            {
                if (extentsDirty)
                {
                    extentsDirty = false;
                    Float2 exMin = points[points.Length - 1].point, 
                           exMax = points[points.Length - 1].point;
                    
                    extents = new Bounds2(exMin, Float2.zero);
                    
                    left = right = top = bottom =
                    smoothLeft = smoothRight = smoothTop = smoothBottom = Length - 1;

                    Float2[] sP = SmoothedPoints;

                    int currentSegment = newIDS[Length - 1];
                    for (int i = points.Length - 1; i >= 0; i--)
                    {

                        if (points[i].point.x < points[left].point.x)
                        {
                            left = i;
                        }
                        else if (points[i].point.x == points[left].point.x &&
                                 points[i].point.y < points[left].point.y)
                        {
                            left = i;
                        }

                        if (points[i].point.y < points[bottom].point.y)
                        {
                            bottom = i;
                        }
                        else if (points[i].point.y == points[bottom].point.y &&
                                 points[i].point.x > points[bottom].point.x)
                        {
                            bottom = i;
                        }

                        if (points[i].point.x > points[right].point.x)
                        {
                            right = i;
                        }
                        else if (points[i].point.x == points[right].point.x &&
                                 points[i].point.y > points[right].point.y)
                        {
                            right = i;
                        }

                        if (points[i].point.y > points[top].point.y)
                        {
                            top = i;
                        }
                        else if (points[i].point.y == points[top].point.y &&
                                 points[i].point.x < points[top].point.x)
                        {
                            top = i;
                        }

                        int nextSegment = newIDS[i];
                        for (int j = currentSegment - 1; j >= nextSegment; j--)
                        {

                            if (sP[j].x < exMin.x)
                            {
                                exMin.x = sP[j].x;
                                smoothLeft = j;

                            }
                            else if (sP[j].x == exMin.x && sP[j].y < exMin.y)
                            {
                                smoothLeft = j;
                            }

                            if (sP[j].y < exMin.y)
                            {
                                exMin.y = sP[j].y;

                                smoothBottom = j;
                            }
                            else if (sP[j].y == exMin.y && sP[j].x > exMax.x)
                            {
                                smoothBottom = j;
                            }

                            if (sP[j].x > exMax.x || (sP[j].x == exMax.x && sP[j].y > exMax.y))
                            {
                                exMax.x = sP[j].x;
                                right = i;
                                smoothRight = j;
                            }
                            if (sP[j].y > exMax.y || (sP[j].y == exMax.y && sP[j].x < exMin.x))
                            {
                                exMax.y = sP[j].y;
                                top = i;
                                smoothTop = j;
                            }
                            
                        }
                        currentSegment = nextSegment;
                    }
                    extents.SetMinMax(exMin, exMax);
                }
                return extents;
            }
        }

        public Float2 Min { get { return min; } }

        public Float2 Max { get { return max; } }

        public Float2 SmoothMin { get { return smoothMin; } }

        public Float2 SmoothMax { get { return smoothMax; } }

        public Float2[] SmoothedPoints
        {
            get
            {
                if (anchorsDirty)
                {
                    SetCapPoints();
                    smoothedPoints = GetSmoothedPoints(out newIDS);
                    anchorsDirty = false;
                }
                return smoothedPoints;
            }
        }

        public override int Length
        {
            get { if (wrap) { return points.Length - 1; } else { return points.Length; } }
        }

        public override bool Wrap
        {
            get { return wrap; }
            set
            {
                if (value != wrap)
                {
                    if (value)
                    {
                        if ((!points[0].point.Equals(points[points.Length - 1].point)))
                        {
                            System.Array.Resize(ref points, points.Length + 1);
                            points[points.Length - 1] = points[0];

                        }
                    }
                    else
                    {
                        if ((points[0].point.Equals(points[points.Length - 1].point)))
                        {
                            System.Array.Resize(ref points, points.Length - 1);
                        }
                    }
                    Sections = points.Length - 1;
                    anchorsDirty = true;
                    extentsDirty = true;
                    SetCapPoints();
                    GetTotalWeight();
                    GetTotalSplineLength(1);
                }
                wrap = value;
            }
        }

        public override int Sections
        {
            get { return sections.Length; }
            set
            {
                if (value != sections.Length)
                {
                    int newLength = value;
                    if (newLength > points.Length - 1)
                    {
                        newLength = points.Length - 1;
                    }
                    if (newLength > 0)
                    {
                        SplineSection[] newSec = new SplineSection[newLength];

                        totalWeight = 0;

                        int lesser = sections.Length;
                        if (newLength < lesser)
                        {
                            lesser = newLength;
                        }
                        for (int i = lesser - 1; i >= 0; i--)
                        {
                            newSec[i] = sections[i];
                        }

                        if (newLength > lesser)
                        {
                            for (int i = lesser; i < newLength; i++)
                            {
                                newSec[i].curveType = defaultCurveType;
                                newSec[i].weight = defaultWeight;
                                totalWeight += defaultWeight;
                            }
                        }
                        sections = newSec;
                    }
                }
            }
        }

        public float TotalSplineLength
        {
            get { if (splineLength < 0) { splineLength = GetTotalSplineLength(0.1f); } return splineLength; }
        }

        #endregion

        #region Constructors
        public Spline2()
        {
            defaultCurveType = CurveType.Linear;
            defaultWeight = 1f;
            defaultDropValue = 0;

            points = new Point2[0];

            Sections = 0;

            splineLength = 0;

            anchorsDirty = extentsDirty = true;
        }

        public Spline2(int p, CurveType dst = CurveType.Linear)
        {
            if (p <= 1) { p = 2; }
            //Debug.Log ("Spline : " + p);

            points = new Point2[p];

            float inv = 1f / p;
            for (int i = p - 1; i >= 0; i--)
            {
                points[i].point = new Float2(inv * i, inv * i);
            }

            defaultCurveType = dst;
            defaultWeight = 1f;
            defaultDropValue = 0;

            Sections = p - 1;
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = extentsDirty = true;
        }
        public Spline2(Float2[] p)
        {
            defaultCurveType = CurveType.Linear;
            defaultWeight = 1f;
            defaultDropValue = 0;

            points = new Point2[p.Length + 1];
            for (int i = p.Length - 1; i >= 0; i--)
            {
                points[i] = new Point2();
                points[i].point = p[i];
            }
            points[p.Length] = points[0];
            Wrap = true;
            Sections = p.Length;

            splineLength = 0;

            anchorsDirty = extentsDirty = true;
        }


        public Spline2(Spline2 spline, int p = -1, bool weighted = true, bool guaranteeResample = false)
        {
            if (p < 0) { p = spline.Length; }
            if (p < 2) { p = 2; }
            spline.GetTotalWeight();
            spline.GetTotalSplineLength(0.01f);
            if (p == spline.Length && !guaranteeResample)
            {

                name = spline.name;
                spline.GetAnchorPoints(out points);
                sections = spline.sections;

                wrap = spline.wrap;
                left = spline.left;
                right = spline.right;
                top = spline.top;
                bottom = spline.bottom;

            }
            else
            {
                points = new Point2[p];

                wrap = spline.wrap;
                float per = 1f / p;
                int s = -1;
                if (p > 2)
                {

                    for (int i = 0; i < p - 1; i++)
                    {
                        s = spline.GetSectionAtPercent(per * i, weighted);

                        points[i].point = spline.GetPoint(per * i, weighted);
                        if (!weighted)
                        {
                            sections[i].weight = 1f;
                        }
                        else
                        {
                            sections[i].weight = per * spline.sections[s].weight;
                        }
                        totalWeight += sections[i].weight;
                        sections[i].curveType = spline.sections[s].curveType;
                    }
                    points[p - 1] = spline.points[spline.Length - 1];
                }
                else
                {
                    points[0] = spline.points[0];
                    sections[0].weight = spline.sections[0].weight;

                    sections[0].curveType = spline.sections[0].curveType;
                    points[1] = spline.points[spline.Length - 1];
                }
                if (wrap && !points[0].point.Equals(points[points.Length - 1].point))
                {
                    System.Array.Resize(ref points, points.Length + 1);
                    points[points.Length - 1] = points[0];

                }
            }
            splineLength = GetTotalSplineLength(0.01f);
            anchorsDirty = extentsDirty = true;
        }
        #endregion

        #region Spline Methods
        public void GetMinMax()
        {
            min = points[points.Length - 1].point;
            max = points[points.Length - 1].point;

            Float2[] sP = SmoothedPoints;

            smoothMin = sP[sP.Length - 1];
            smoothMax = sP[sP.Length - 1];

            for (int i = Length - 1; i >= 0; i--)
            {

                if (points[i].point.x < min.x)
                {
                    min.x = points[i].point.x;
                }
                else if (points[i].point.x > max.x)
                {
                    max.x = points[i].point.x;
                }

                if (points[i].point.y < min.y)
                {
                    min.y = points[i].point.y;
                }
                else if (points[i].point.y > max.y)
                {
                    max.y = points[i].point.y;
                }
            }

            for (int i = sP.Length - 1; i >= 0; i--)
            {

                if (sP[i].x < smoothMin.x)
                {
                    smoothMin.x = sP[i].x;
                }
                else if (sP[i].x > smoothMax.x)
                {
                    smoothMax.x = sP[i].x;
                }

                if (sP[i].y < smoothMin.y)
                {
                    smoothMin.y = sP[i].y;
                }
                else if (sP[i].y > smoothMax.y)
                {
                    smoothMax.y = sP[i].y;
                }
            }
        }

        public void SetAnchorPoints(Point2[] newPoints)
        {
            points = newPoints;
            if (wrap && !points[0].point.Equals(points[points.Length - 1].point))
            {
                System.Array.Resize(ref points, points.Length + 1);
                points[points.Length - 1] = points[0];
            }


            Sections = points.Length - 1;
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = extentsDirty = true;
        }

        public void GetAnchorPoints(out Point2[] p)
        {
            int l = points.Length;
            p = new Point2[l];
            for (int i = points.Length - 1; i >= 0; i--)
            {
                p[i] = points[i];
            }
        }

        public override void Scale(float scalar)
        {
            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i].point *= scalar;
                if (points[i].useBackHandle) points[i].backHandle *= scalar;
                if (points[i].useForeHandle) points[i].foreHandle *= scalar;
            }
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = true;
            extentsDirty = true;
        }

        public override void Normalize()
        {
            GetMinMax();
            Float2 offset = smoothMin; offset *= -1;
            Float2 scalar = smoothMax - smoothMin;
            if (scalar.x == 0) { scalar.x = 1; } else { scalar.x = 1f / scalar.x; }
            if (scalar.y == 0) { scalar.y = 1; } else { scalar.y = 1f / scalar.y; }

            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i].point += offset;
                points[i].point = points[i].point * scalar;

                if (points[i].useBackHandle) points[i].backHandle = points[i].backHandle * scalar;
                if (points[i].useForeHandle) points[i].foreHandle = points[i].foreHandle * scalar;
            }
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = true;
        }

        public void SetXOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.x = vals[i];
            }
        }

        public void SetYOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.y = vals[i];
            }
        }

        public void AddPoints(Float2[] p, CurveType[] curves, float[] weights, int id = -1)
        {
            //Revamp this to do all the adding work itself, instead of repeatedly calling AddPoint.
            int npc = points.Length;
            int fillerSlots = 0;
            if (id > npc) { fillerSlots = id - npc; }

            totalWeight = 0;
            List<Point2> newPoints = new List<Point2>(points);
            List<SplineSection> newSS = new List<SplineSection>(sections);

            for (int i = 0; i < p.Length; i++)
            {

                CurveType curve = curves[i];
                float weight = weights[i];
                if (curves[i] == CurveType.None)
                {
                    curve = defaultCurveType;
                }
                if (weights[i] < 0)
                {
                    weight = defaultWeight;
                }

                if (id < 0)
                {
                    Point2 p2 = new Point2(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.weight = weight;
                    ss.curveType = curve;
                    newPoints.Add(p2); newSS.Add(ss);
                    totalWeight += weight;

                }
                else if (id < points.Length)
                {
                    Point2 p2 = new Point2(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.curveType = curve;
                    newPoints.Insert(id, p2); newSS.Insert(id, ss);
                    totalWeight += weight;

                }
                else
                {
                    for (int j = 0; j < fillerSlots; j++)
                    {
                        SplineSection se = new SplineSection();
                        se.weight = defaultWeight;
                        se.curveType = defaultCurveType;
                        newPoints.Add(new Point2()); newSS.Add(se);
                        totalWeight += defaultWeight;

                    }
                    Point2 p2 = new Point2(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.weight = weight;
                    ss.curveType = curve;
                    newPoints.Add(p2); newSS.Add(ss);

                }
            }
            points = newPoints.ToArray();
            sections = newSS.ToArray();

            anchorsDirty = true;
            extentsDirty = true;
            SetCapPoints();
            GetTotalWeight();
            GetTotalSplineLength(1);
        }

        public void AddPoint(Float2 p, int id = -1, CurveType s = CurveType.None, float t = -1f)
        {
            if (s == CurveType.None)
            {
                s = defaultCurveType;
            }
            if (t < 0)
            {
                t = defaultWeight;
            }


            int npc = points.Length;
            int fillerSlots = 0;
            if (id > npc) { fillerSlots = id - npc; }

            totalWeight = 0;
            List<Point2> newPoints = new List<Point2>(points);
            List<SplineSection> newSS = new List<SplineSection>(sections);


            if (id < 0)
            {
                Point2 p2 = new Point2(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.curveType = s;
                ss.weight = t;
                newPoints.Add(p2); newSS.Add(ss);
                totalWeight += t;

            }
            else if (id < points.Length)
            {
                Point2 p2 = new Point2(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.weight = t;
                ss.curveType = s;
                newPoints.Insert(id, p2); newSS.Insert(id, ss);
                totalWeight += t;

            }
            else
            {
                for (int i = 0; i < fillerSlots; i++)
                {
                    SplineSection se = new SplineSection();
                    se.weight = defaultWeight;
                    se.curveType = defaultCurveType;
                    newPoints.Add(new Point2()); newSS.Add(se);
                    totalWeight += defaultWeight;

                }
                Point2 p2 = new Point2(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.weight = t;
                ss.curveType = s;
                newPoints.Add(p2); newSS.Add(ss);

            }


            points = newPoints.ToArray();

            sections = newSS.ToArray();

            anchorsDirty = true;
            extentsDirty = true;
            SetCapPoints();
            GetTotalWeight();
            GetTotalSplineLength(1);
        }

        public override void RemovePoint(int id)
        {

            if (id >= 0 && id < points.Length)
            {
                List<Point2> newPoints = new List<Point2>(points);

                List<SplineSection> newSections = new List<SplineSection>(sections);

                newPoints.RemoveAt(id);
                if (id < points.Length - 1)
                {
                    newSections.RemoveAt(id);
                }
                points = newPoints.ToArray();

                sections = newSections.ToArray();

                totalWeight = 0;
                for (int i = sections.Length - 1; i >= 0; i--)
                {
                    totalWeight += sections[i].weight;
                }
            }
        }

        public override void RemovePoints(int[] ids)
        {
            List<Point2> newPoints = new List<Point2>(points);
            List<SplineSection> newSections = new List<SplineSection>(sections);

            for (int i = Length - 1; i >= 0; i--)
            {

                for (int j = 0; j < ids.Length; j++)
                {
                    if (i == ids[j])
                    {
                        newPoints.RemoveAt(ids[j]);
                        if (i < points.Length - 1) newSections.RemoveAt(ids[j]);

                    }
                }

            }

            points = newPoints.ToArray();

            sections = newSections.ToArray();

            totalWeight = 0;
            for (int i = sections.Length - 1; i >= 0; i--)
            {
                totalWeight += sections[i].weight;
            }
        }

        public override void SetCapPoints()
        {
            //First, how many points do we actually have?
            int l = points.Length;

            //We need at least two points.
            if (l > 1)
            {
                //Our starting control point is the difference between point 0 and point 1 subtracted from point 0.
                Float2 pointDiff = points[1].point; pointDiff -= points[0].point;
                startCap = points[0].point; startCap -= pointDiff;

                if (l >= 3)
                {
                    //We check to see if this is a loop - if the first and last official points are the same.
                    // If so, we set our control points to form that perfect, continuous loop.
                    if (points[0].point.Equals(points[l - 1].point))
                    {
                        startCap = points[l - 2].point;
                        endCap = points[1].point;
                    }
                    //If not, we create our two control points. Each is just a straight line from our second set of points, going inward, through our first 
                    //set of points.	
                    else
                    {
                        pointDiff = points[l - 1].point - points[l - 2].point;
                        endCap = points[l - 1].point; endCap += pointDiff;
                    }
                }
                else
                {
                    endCap = points[1].point; endCap += pointDiff;
                }
            }
        }

        public Float2 GetPoint(float percent, bool weighted = true)
        {

            Float2 currentPoint;
            if (points.Length > 0)
            {
                if (points.Length > 1)
                {
                    if (percent <= 0)
                    {
                        currentPoint = points[0].point;
                    }
                    else if (percent >= 1)
                    {
                        currentPoint = points[points.Length - 1].point;
                    }
                    else
                    {
                        int cS = GetSectionAtPercent(percent, weighted);

                        if (cS != currentSection)
                        {
                            currentSection = cS;
                            GetControlPoints();
                        }
                        Float2 point = Float2.zero;
                        switch (sections[cS].curveType)
                        {
                            //Catmull Rom
                            case CurveType.CatmullRom:
                                point.x = Splines.CatmullRom(weightedPercent, a.x, b.x, c.x, d.x);
                                point.y = Splines.CatmullRom(weightedPercent, a.y, b.y, c.y, d.y);
                                break;

                            //Linear
                            case CurveType.Linear:
                                point = Float2.Lerp(points[cS].point, points[cS + 1].point, weightedPercent);

                                break;

                            //CubicBezier
                            case CurveType.CubicBezier:
                                point.x = Splines.CubicBezier(weightedPercent, a.x, b.x, c.x, d.x);
                                point.y = Splines.CubicBezier(weightedPercent, a.y, b.y, c.y, d.y);
                                break;

                            //Step
                            case CurveType.Step:
                                point = points[cS].point;
                                break;
                        }
                        currentPoint = point;
                    }

                }
                else
                {
                    currentPoint = points[0].point;
                }
            }
            else
            {
                currentPoint = Float2.zero;
            }
            return currentPoint;
        }

        public override void GetControlPoints()
        {
            int l = sections.Length;
            if (currentSection == 0 && l > 1)
            {
                a = startCap;
                b = points[0].point;
                c = points[1].point;
                d = points[2].point;
            }
            else if (currentSection == 0)
            {
                a = startCap;
                b = points[0].point;
                c = points[1].point;
                d = endCap;
            }
            else if (currentSection == l - 1)
            {
                a = points[l - 2].point;
                b = points[l - 1].point;
                c = points[l].point;
                d = endCap;
            }
            else if (currentSection > 0)
            {
                a = points[currentSection - 1].point;
                b = points[currentSection].point;
                c = points[currentSection + 1].point;
                d = points[currentSection + 2].point;
            }

            if (points[currentSection].useBackHandle)
            {
                a = points[currentSection].point + points[currentSection].backHandle;
            }
            if (points[currentSection + 1].useForeHandle)
            {
                d = points[currentSection + 1].point + points[currentSection + 1].foreHandle;
            }
        }

        public Spline2 SplitSpline(int id, bool includeCurrent)
        {

            if (points.Length > id)
            {
                List<Point2> newPoints = new List<Point2>(points);

                List<SplineSection> newSections = new List<SplineSection>(sections);

                int diff = Length - id;
                List<Point2> splitPoints = newPoints.GetRange(id, diff);
                List<SplineSection> splitSections = newSections.GetRange(id, diff);


                points = newPoints.GetRange(0, id - 1).ToArray();
                sections = newSections.GetRange(0, id - 1).ToArray();

                totalWeight = 0;
                for (int i = sections.Length - 1; i >= 0; i--)
                {
                    totalWeight += sections[i].weight;
                }

                Spline2 leftOvers = new Spline2();
                leftOvers.SetAnchorPoints(splitPoints.ToArray());
                leftOvers.sections = splitSections.ToArray();

                leftOvers.totalWeight = 0;
                for (int i = leftOvers.sections.Length - 1; i >= 0; i--)
                {
                    leftOvers.totalWeight += leftOvers.sections[i].weight;
                }

                return leftOvers;
            }
            else
            {
                return new Spline2();
            }
        }

        public Spline2 GetSubSpline(int start, int end)
        {
            List<Point2> newPoints = new List<Point2>(points);

            List<SplineSection> newSections = new List<SplineSection>(sections);

            int diff = end - start;
            int ddif = diff - 1;
            Spline2 s = new Spline2();
            if (start > end)
            {
                if (!wrap) { return null; }
                //diff = points.Length - 1 - start;
                diff = points.Length - start;
                ddif = diff - 1;
                newPoints = newPoints.GetRange(start, diff);
                newSections = newSections.GetRange(start, ddif);

                for (int i = 1; i <= end; i++)
                {
                    newPoints.Add(points[i]);

                    newSections.Add(sections[i]);

                }
            }
            else
            {
                newPoints = newPoints.GetRange(start, diff);
                newSections = newSections.GetRange(start, ddif);
            }

            s.points = newPoints.ToArray();
            s.sections = newSections.ToArray();

            if (start > 0)
            {
                s.startCap = points[start - 1].point;

            }
            else { s.startCap = startCap; }
            if (end < points.Length - 1)
            {
                s.endCap = points[end + 1].point;

            }
            else { s.endCap = new Float2(endCap.x, endCap.y); }

            s.anchorsDirty = true;

            s.Sections = newSections.Count;
            s.GetTotalWeight();
            s.GetTotalSplineLength(0.01f, true);
            return s;
        }

        //I should get either ClosestPolyPoint or this working for both circumstances. Start by checking to see which works better.
        public int GetClosestAnchorPoint(Float2 p)
        {
            float lowestDist = float.PositiveInfinity;
            int id = -1;
            for (int i = Length - 1; i >= 0; i--)
            {

                float d = Float2.Distance(p, this[i]);
                if (d < lowestDist)
                {
                    id = i;
                    lowestDist = d;
                }
            }
            return id;
        }

        public Float2 ClosestPointOnSpline(Float2 point, int samples, ref int closestAnchor, bool weighted = true)
        {
            closestAnchor = points.Length - 2;

            float currentDistance = Float2.Distance(point, points[points.Length - 1].point),
                  currentPer = 1f,
                  per,
                  nextPer;

            Float2 p1, p2 = points[points.Length - 2].point, p3 = points[points.Length - 1].point;

            nextPer = GetPercentAtPoint(points.Length - 1, weighted);

            for (int i = points.Length - 2; i >= 0; i--)
            {
                per = GetPercentAtPoint(i, weighted);
                float incr = (nextPer - per) / samples;

                for (int j = samples - 1; j >= 0; j--)
                {
                    Float2 tp2 = GetPoint(per + (incr * j));
                    float dist =Float2.Distance(point, tp2);
                    if (dist < currentDistance)
                    {
                        p2 = tp2;
                        currentDistance = dist;
                        currentPer = per + (incr * j);
                        closestAnchor = i;
                    }
                }
                nextPer = per;
            }
            p1 = points[closestAnchor].point;
            p3 = points[closestAnchor + 1].point;


            Float2 intersect1 = new Float2(),
                    intersect2 = new Float2(),
                    otherPoint = Extents.Center,
                    p4;
            Float2 p5 = (otherPoint - point).Normalize();

            p4 = otherPoint + (p5 * 100000);
            p5 = otherPoint - (p5 * 100000);

            per = GetPercentAtPoint(closestAnchor, weighted);
            if (Float2.LineIntersection(p1, p2, p4, p5, ref intersect1))
            {
                if (Float2.LineIntersection(p2, p3, p4, p5, ref intersect2))
                {
                    float dist1 = Float2.Distance(intersect1, point),
                          dist2 = Float2.Distance(intersect2, point);

                    if (dist1 < dist2)
                    {
                        p3 = p2;
                        nextPer = currentPer;
                    }
                    else
                    {
                        p1 = p2;
                        per = currentPer;
                        nextPer = GetPercentAtPoint(closestAnchor + 1, weighted);
                    }

                }
                else
                {
                    p3 = p2;
                    nextPer = currentPer;
                }
            }
            else if (Float2.LineIntersection(p2, p3, p4, p5, ref intersect2))
            {
                p1 = new Float2(p2.x, p2.y);
                per = currentPer;
                nextPer = GetPercentAtPoint(closestAnchor + 1, weighted);
            }

            currentPer = (nextPer + per) * 0.5f;
            p2 = GetPoint(currentPer, weighted);

            for (int i = samples; i > 0; i--)
            {
                float dist1 = Float2.Distance(p1, point),
                      dist2 = Float2.Distance(p3, point);
                if (dist1 < dist2)
                {
                    p3 = p2;
                    nextPer = currentPer;
                    currentPer = (per + currentPer) * 0.5f;
                }
                else
                {
                    p1 = p2;
                    per = currentPer;
                }
                currentPer = (nextPer + per) * 0.5f;
                p2 = GetPoint(currentPer, weighted);
            }
            return p2;
        }

        public override float GetTotalSplineLength(float percent, bool resetHeldValue = false)
        {

            float sLength = 0f;

            if (sections.Length > 0)
            {

                float secPer = 0;
                float sp = 0;
                for (int j = 0; j < sections.Length; j++)
                {
                    sections[j].length = 0;
                    if (sections[j].curveType == CurveType.Linear)
                    {

                        sections[j].length = Float2.Distance(points[j].point, points[j + 1].point);
                        sLength += sections[j].length;
                    }
                    else if (sections[j].curveType == CurveType.CatmullRom || sections[j].curveType == CurveType.CubicBezier)
                    {

                        Float2 a = points[j].point, b;
                        secPer += sections[j].weight;
                        float isp = secPer / totalWeight;

                        for (float i = percent; i <= 1; i += percent)
                        {
                            b = GetPoint(Float.Lerp(sp, isp, i));
                            float d = Float2.Distance(a, b);
                            a = b;
                            sections[j].length += d;
                            sLength += d;
                        }
                        sp = isp;
                    }
                }
            }
            if (resetHeldValue) { splineLength = sLength; }
            return sLength;
        }

        public Float2 AveragePoints(bool useSmoothed = true)
        {

            Float2 average = Float2.zero;

            if (useSmoothed)
            {
                Float2[] pts = SmoothedPoints;
                int l = pts.Length;
                if (wrap) { l--; }
                for (int i = 0; i < l; i++)
                {
                    average += pts[i];
                }
                average /= l;
            }
            else
            {
                int p = points.Length;
                if (wrap) { p--; }
                for (int i = 0; i < p; i++)
                {
                    average += points[i].point;
                }
                average /= p;
            }

            return average;
        }

        public void Offset(Float2 offset)
        {
            for (int i = Length - 1; i >= 0; i--)
            {
                this[i] += offset;
            }
            anchorsDirty = true;
            extentsDirty = true;
        }

        public override void ResamplePoints(int samples)
        {
            if (samples > 2)
            {

                float perc = 1f / (samples - 1);

                Point2[] newPoints = new Point2[samples];
                SplineSection[] secs = new SplineSection[samples - 1];

                newPoints[0] = points[0];
                secs[0] = sections[0];

                int s = 0;
                for (int i = 1; i < points.Length; i++)
                {
                    float ip = GetPercentAtPoint(i), sp = s * perc;
                    while (sp < ip)
                    {
                        s++;
                        newPoints[s] = points[i];
                        newPoints[s].point = GetPoint(sp);
                        if (i < points.Length - 1) { secs[s] = sections[i]; }
                        sp = s * perc;
                    }
                    if (s == samples) { break; }
                }

                points = newPoints;
                sections = secs;

            }
        }

        public override float Distance(float p1, float p2)
        {
            return Distance(p1, p2, 20);
        }

        public override float Distance(float p1, float p2, int samples)
        {
            float distance = 0f;
            float p = p2 - p1 / samples;
            float cP = p1;
            Float2 currentPoint = GetPoint(p1);
            Float2 prevPoint = currentPoint;

            for (int i = samples - 1; i >= 0; i--)
            {
                cP += p;
                currentPoint = GetPoint(cP);
                distance += Float2.Distance(prevPoint, currentPoint);
                prevPoint = currentPoint;
            }
            return distance;
        }

        public Float2[] GetSmoothedPoints(out int[] newIDS, int maxSubdivisions = 10, float subDivideAngle = 15)
        {
            if(Length == 0) { newIDS = new int[0]; return new Float2[0]; }

            if (maxSubdivisions < 3) { maxSubdivisions = 3; }
            //Debug.Log("smoothing " + name);
            List<Float2> newPoints = new List<Float2>();
            List<int> nIDS = new List<int>();
            //Debug.Log((points.Length - 2) + " | " + sectionTypes.Length);
            for (int i = 0; i < points.Length - 1; i++)
            {
                nIDS.Add(newPoints.Count);
                newPoints.Add(points[i].point);
                //Debug.Log("Switching section type " + i);
                //Debug.Log("Total Weight:" + totalWeight);
                switch (sections[i].curveType)
                {
                    case CurveType.CatmullRom:
                    case CurveType.CubicBezier:

                        currentSection = -1;
                        float pa = GetPercentAtPoint(i), pb = GetPercentAtPoint(i + 1);
                        //Debug.Log(i + ". PA: " + pa + "  PB: " + pb);
                        float diff = pb - pa;
                        float per = diff / maxSubdivisions;

                        for (int j = 1; j < maxSubdivisions; j++)
                        {
                            newPoints.Add(GetPoint(pa + (per * j)));
                        }

                        break;
                }
            }
            nIDS.Add(newPoints.Count);
            newPoints.Add(points[points.Length - 1].point);

            newIDS = nIDS.ToArray();
            return newPoints.ToArray();
        }

        public override int GetSectionID(int smoothID)
        {

            int currentSec = newIDS[points.Length - 1];
            if (smoothID <= currentSec)
            {
                for (int i = points.Length - 2; i >= 0; i--)
                {
                    if (smoothID < currentSec)
                    {
                        return i;
                    }
                    currentSec = newIDS[i];
                }
                return points.Length - 1;
            }
            return -1;
        }

        public bool SelfIntersection(Float2 a, int id)
        {
            Float2[] pts = SmoothedPoints;

            Float2 b1, b2, c, d, e = Float2.zero;
            id = newIDS[id];
            if (id == 0) { b1 = pts[pts.Length - 2]; b2 = pts[1]; }
            else { b1 = pts[id - 1]; b2 = pts[id + 1]; }

            // Run two for loops. For everything greater than id 
            // but less than length-1, and for everything less that id
            // but greater than zero;

            for (int j = pts.Length - 3; j > id; j--)
            {
                c = pts[j]; d = pts[j + 1];
                if (Float2.LineIntersection(a, b1, c, d, ref e) ||
                    Float2.LineIntersection(a, b2, c, d, ref e))
                {
                    return true;
                }

            }
            for (int j = id - 2; j >= 0; j--)
            {
                c = pts[j]; d = pts[j + 1];
                if (Float2.LineIntersection(a, b1, c, d, ref e) ||
                    Float2.LineIntersection(a, b2, c, d, ref e))
                {
                    return true;
                }
            }
            return false;
        }

        public void RotateSpline(float angle)
        {
            Float2 c = extents.Center;
            for (int i = points.Length - 1; i >= 0; i--)
            {
                Float2 p = points[i].point;
                points[i].point = p.RotateAround(c, angle);
            }
        }

        public void ScaleSpline(Float2 scale)
        {
            Float2 c = extents.Center;
            for (int i = points.Length - 1; i >= 0; i--)
            {
                Float2 p = points[i].point;
                p -= c;
                p = p * scale;
                p += c;
                points[i].point = p;
            }
        }
        #endregion

        #region Polygon Methods
        public static Spline2 NewPolygon(int sides = 3, float radius = 0.5f, float angleOffset = 0f)
        {
            if (sides < 3) { sides = 3; }
            Float2[] poly = Splines.RegularPolygon(sides, radius, angleOffset);
            poly = poly.Reverse().ToArray();

            Spline2 polygon = new Spline2(poly);
            polygon.Wrap = true;
            polygon.extentsDirty = true;

            return polygon;
        }

        public static Spline2 NewStar(int sides = 3, float pointRadius = 0.5f, float innerRadius = 0.1f)
        {
            if (sides < 3) { sides = 3; }
            Float2[] poly = Splines.RegularStar(sides, pointRadius, innerRadius);

            Spline2 polygon = new Spline2(poly);
            polygon.Wrap = true;
            polygon.extentsDirty = true;

            return polygon;
        }

        public static Spline2 NewFromRect(Bounds2 rect)
        {
            return NewRectangle(rect.Center, rect.Size);
        }
        public static Spline2 NewRectangle(Float2 center, Float2 size)
        {
            Spline2 rectangle = new Spline2(4);
            float xMin = center.x - size.x * 0.5f, yMin = center.y - size.y * 0.5f;
            float xMax = xMin + size.x, yMax = yMin + size.y;

            rectangle[0] = new Float2(xMin, yMin);
            rectangle[1] = new Float2(xMin, yMax);
            rectangle[2] = new Float2(xMax, yMax);
            rectangle[3] = new Float2(xMax, yMin);
            rectangle.Wrap = true;
            return rectangle;
        }


        /// <summary>
        /// Treating the spline as a polygon, does it contain the supplied point?
        /// </summary>
        /// <param name="p">The point to check.</param>
        /// <returns>True if the poly contains the point, False otherwise.</returns>
        public bool ContainsPoint(Float2 p)
        {
            if (!wrap) { return false; }

            bool inside = Extents.Contains(p);
            if (inside)
            {
                inside = false;
                Float2[] sP = SmoothedPoints;
                for (int i = 0; i < sP.Length - 1; i++)
                {
                    Float2 pi = sP[i];
                    Float2 pj = sP[i + 1];

                    if (((pi.y <= p.y && p.y < pj.y) ||
                        (pj.y <= p.y && p.y < pi.y))
                        &&
                        (p.x < (pj.x - pi.x) * (p.y - pi.y) / (pj.y - pi.y) + pi.x))
                    {
                        inside = !inside;
                    }

                }

                return inside;
            }
            return false;
        }

        public static bool ContainsPoint(Float2 p, Float2[] poly)
        {

            bool inside = false;
            Float2 pi = Float2.zero, pj = Float2.zero;
            for (int i = 0; i < poly.Length - 1; i++)
            {
                pi = poly[i];
                pj = poly[i + 1];

                if (((pi.y <= p.y && p.y < pj.y) ||
                    (pj.y <= p.y && p.y < pi.y))
                    &&
                    (p.x < (pj.x - pi.x) * (p.y - pi.y) /
                    (pj.y - pi.y) + pi.x))
                {
                    inside = !inside;
                }

            }
            pi = poly[poly.Length - 1];
            pj = poly[0];

            if (((pi.y <= p.y && p.y < pj.y) ||
                (pj.y <= p.y && p.y < pi.y))
                &&
                (p.x < (pj.x - pi.x) * (p.y - pi.y) /
                (pj.y - pi.y) + pi.x))
            {
                inside = !inside;
            }
            return inside;
        }

        public bool SharesPoint(Float2 point, float tolerance = 0.01f)
        {
            for (int i = Length - 1; i >= 0; i--)
            {
                if (points[i].point.NearlyEqual(point, tolerance))
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Treating each spline as a polygon, checks to see if one Spline2 contains another.
        /// </summary>
        /// <param name="p">The other Polygon</param>
        /// <returns> 
        /// 0 means the polys do not intersect at all.
        /// 1 means this poly fully contains the other.
        /// -1 means the other poly fully contains this one.
        /// 2 means they intersect, but do not fully contain one another. 
        /// </returns>
        public int ContainsPolygon(Spline2 p)
        {

            //Check all line segmenets against each other. 
            Float2[] pts = p.SmoothedPoints;
            Float2[] tpts = SmoothedPoints;
            for (int i = tpts.Length - 1; i >= 0; i--)
            {
                Float2 p2 = tpts[i], p1, p3, p4, intersect = Float2.zero;
                if (i == 0) { p1 = tpts[Length - 1]; } else { p1 = tpts[i - 1]; }

                for (int j = pts.Length - 1; j >= 0; j--)
                {
                    p4 = pts[j];
                    if (j == 0) { p3 = pts[pts.Length - 1]; } else { p3 = pts[j - 1]; }

                    if (Float2.LineIntersection(p1, p2, p3, p4, ref intersect))
                    {
                        return 2;
                    }
                }
            }
            //If there are no intersections, check any point in p. If it's inside
            //this Polygon, we contain that whole Polygon.
            if (ContainsPoint(pts[0])) { return 1; }
            else if (p.ContainsPoint(this[0])) { return -1; }

            return 0;
        }

        public Float2 GetClosestPointOnPolygon(Float2 p, int samples, ref int cls)
        {
            int closestPolyPoint = GetClosestAnchorPoint(p), left = -1, right = -1;
            Float2 ret = Float2.one * -1, a, b;
            int l = Length - 1, sec;
            float dist;

            if (closestPolyPoint == 0)
            {

                left = l;
                right = 1;
            }
            else if (closestPolyPoint == l)
            {
                right = 0;
                left = l - 1;

            }
            else
            {
                right = closestPolyPoint + 1;
                left = closestPolyPoint - 1;
            }

            a = this[closestPolyPoint];
            dist = Float2.Distance(p, this[left]);
            float perA, perB;

            if (Float2.Distance(p, this[right]) > dist)
            {
                b = a;
                sec = left;
                a = this[left];
                cls = left;
                perA = GetPercentAtPoint(left);
                perB = GetPercentAtPoint(closestPolyPoint);
            }
            else
            {
                b = this[right];
                sec = closestPolyPoint;
                cls = closestPolyPoint;
                perA = GetPercentAtPoint(closestPolyPoint);
                perB = GetPercentAtPoint(right);
            }

            CurveType secT = sections[sec].curveType;
            switch (secT)
            {
                case CurveType.Linear:

                    for (int s = samples; s > 0; s--)
                    {
                        ret = Float2.Lerp(a, b, 0.5f);
                        dist = Float2.Distance(a, p);
                        if (Float2.Distance(b, p) < dist)
                        {
                            a = ret;
                        }
                        else
                        {
                            b = ret;
                        }
                    }
                    break;
                case CurveType.CatmullRom:
                case CurveType.CubicBezier:


                    for (int s = samples; s > 0; s--)
                    {
                        float percentForSample = perA + 0.5f * (perB - perA);
                        ret = GetPoint(percentForSample);
                        dist = Float2.Distance(a, p);
                        if (Float2.Distance(b, p) < dist)
                        {
                            a = ret;
                            perA = percentForSample;
                        }
                        else
                        {
                            b = ret;
                            perB = percentForSample;
                        }
                    }
                    break;
            }
            dist = Float2.Distance(a, p);
            if (dist < Float2.Distance(b, p))
            {
                ret = a;
            }
            else
            {
                ret = b;
            }

            return ret;
        }

        public bool LineIntersects(Float2 a, Float2 b) //, ref int aid, ref int bid) {
        {
            Float2 v = Float2.zero;
            for (int i = Length - 1; i >= 0; i--)
            {
                if (Float2.LineIntersection(points[i].point, points[i + 1].point, a, b, ref v))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Gets the line segments and points of intersection between this Spline2 and another.
        /// </summary>
        /// <param name="other">The Spline2 to check against this one.</param>
        /// <param name="lineSegments">Comes in a set of four integers. The first two are this line segment, 
        /// the second two are the other line segment. </param>
        /// <param name="intersectionPoints">The coordinates of intersection to be matched with lineSegments.</param>
        /// <param name="segmentPercents">The percent along each line where a collision point lies.</param>
        public void GetIntersections(Spline2 other, out int[] lineSegments, out Float2[] intersectionPoints, out float[] segmentPercents)
        {

            List<int> lineSegs = new List<int>();
            List<Float2> intersects = new List<Float2>();
            List<float> segPer = new List<float>();
            Float2[] myPoints = SmoothedPoints, otherPoints = other.SmoothedPoints;
            Float2 intersect = Float2.zero;

            for (int i = 0; i < myPoints.Length - 1; i++)
            {

                int a = i, b = i + 1, c, d;

                int abCount = 0;
                Float2 p1 = myPoints[a], p2 = myPoints[b], p3, p4;

                for (int j = 0; j < otherPoints.Length - 1; j++)
                {
                    c = j;
                    d = j + 1;
                    p3 = otherPoints[c];
                    p4 = otherPoints[d];
                    if (Float2.LineIntersection(p1, p2, p3, p4, ref intersect))
                    {
                        float abPercent = Float2.Distance(intersect, p1) / Float2.Distance(p1, p2),
                            cdPercent = Float2.Distance(intersect, p3) / Float2.Distance(p3, p4);

                        if (abCount > 0)
                        {
                            // count back through our recent additions by abCount.
                            bool addAtBeginning = true;
                            for (int abC = 1; abC <= abCount; abC++)
                            {
                                int slotID = intersects.Count - abC;
                                // If the segPer in that slot is LOWER than abPercent,
                                // add after that slot.
                                if (segPer[slotID * 2] < abPercent)
                                {
                                    lineSegs.Insert(slotID * 5, a); lineSegs.Insert(slotID * 5 + 1, b);
                                    lineSegs.Insert(slotID * 5 + 2, c); lineSegs.Insert(slotID * 5 + 3, d);
                                    segPer.Insert(slotID * 3, abPercent); 
                                    segPer.Insert(slotID * 3 + 1, cdPercent);
                                    intersects.Insert(slotID + 1, intersect);
                                    addAtBeginning = false;
                                    break;
                                }
                            }
                            if (addAtBeginning)
                            {
                                lineSegs.Insert(0, a); lineSegs.Insert(1, b);
                                lineSegs.Insert(2, c); lineSegs.Insert(3, d);
                                segPer.Insert(0, abPercent); segPer.Insert(1, cdPercent);
                                intersects.Insert(0, intersect);
                            }
                        }
                        else
                        {

                            lineSegs.Add(a); lineSegs.Add(b);
                            lineSegs.Add(c); lineSegs.Add(d);
                            segPer.Add(abPercent); segPer.Add(cdPercent);
                            intersects.Add(intersect);
                        }
                        abCount++;
                    }
                }
            }
            lineSegments = lineSegs.ToArray();
            intersectionPoints = intersects.ToArray();
            segmentPercents = segPer.ToArray();
        }

        /// <summary>
        /// Sequences a group of intersection points as this Spline2 would encounter them, in clockwise order.
        /// </summary>
        /// <param name="lineSegs">Each set of 4 represents point indices for line AB and line CD</param>
        /// <param name="intersectionPoints">Represents a point of intersection between lines AB and CD.</param>
        /// <param name="segmentPercents">Each set of 2 represents the intersection as a percent along lines AB and CD</param>
        /// <param name="ab">If true, this polygon is represented by each AB line in these intersects. If false, this polygon represents all CD lines.</param>
        public void SequenceIntersects(ref int[] lineSegs, ref Float2[] intersectionPoints, ref float[] segmentPercents, bool ab = true)
        {
            // In cases where we're sequencing for CD lines, we need to 
            List<int> sequencedLineSegs = new List<int>();
            List<Float2> sequencedIntersects = new List<Float2>();
            List<float> sequencedSegPers = new List<float>();
            for (int i = 0; i < SmoothedPoints.Length - 1; i++)
            {
                List<int> newLineSegs = new List<int>();
                List<Float2> newIntersects = new List<Float2>();
                List<float> newSegPers = new List<float>();

                //Sequence all of them as though for AB, just as we had been,
                // but care about our OTHER percents, down below. Sort by them.
                for (int j = 0; j < intersectionPoints.Length; j++)
                {
                    int k = j * 4;
                    if (!ab) { k = j * 4 + 2; }

                    if (lineSegs[k] == i)
                    {
                        newIntersects.Add(intersectionPoints[j]);
                        k = j * 2;
                        newSegPers.Add(segmentPercents[k]);
                        newSegPers.Add(segmentPercents[k + 1]);
                        k = j * 4;
                        newLineSegs.Add(lineSegs[k]);
                        newLineSegs.Add(lineSegs[k + 1]);
                        newLineSegs.Add(lineSegs[k + 2]);
                        newLineSegs.Add(lineSegs[k + 3]);
                    }
                }
                while (newIntersects.Count > 0)
                {
                    int lowestIntersect = -1;
                    float lowestPercent = float.MaxValue;
                    for (int j = newIntersects.Count - 1; j >= 0; j--)
                    {
                        int k;
                        if (ab)
                        {
                            k = j * 2;
                        }
                        else
                        {
                            k = j * 2 + 1;
                        }
                        if (newSegPers[k] < lowestPercent)
                        {
                            lowestPercent = newSegPers[k];
                            lowestIntersect = j;
                        }
                    }

                    int ls = lowestIntersect * 4;
                    sequencedLineSegs.Add(newLineSegs[ls]);
                    sequencedLineSegs.Add(newLineSegs[ls + 1]);
                    sequencedLineSegs.Add(newLineSegs[ls + 2]);
                    sequencedLineSegs.Add(newLineSegs[ls + 3]);
                    newLineSegs.RemoveRange(ls, 4);

                    ls = lowestIntersect * 2;
                    sequencedSegPers.Add(newSegPers[ls]);
                    sequencedSegPers.Add(newSegPers[ls + 1]);
                    newSegPers.RemoveRange(ls, 2);

                    sequencedIntersects.Add(newIntersects[lowestIntersect]);
                    newIntersects.RemoveAt(lowestIntersect);
                }
            }

            lineSegs = sequencedLineSegs.ToArray();
            intersectionPoints = sequencedIntersects.ToArray();
            segmentPercents = sequencedSegPers.ToArray();
        }


        public Spline2[] CombinePolygons(Spline2 other, int curveSampling = 5)
        {
            List<Spline2> final = new List<Spline2>();

            int[] lineSegs;
            Float2[] intersects;
            float[] intersectPercents;

            GetIntersections(other, out lineSegs, out intersects, out intersectPercents);

            if (intersects.Length == 0)
            {

                if (ContainsPoint(other[0])) { final.Add(new Spline2(this)); }
                else if (other.ContainsPoint(this[0])) { final.Add(new Spline2(other)); }
                else { final.Add(new Spline2(this)); final.Add(new Spline2(other)); }
            }
            else
            {
                List<Point2> thesePoints = new List<Point2>(points), otherPoints = new List<Point2>(other.points);

                List<SplineSection> theseSections = new List<SplineSection>(sections),
                                  otherSections = new List<SplineSection>(other.sections);

                List<int> thesePointStates = new List<int>(new int[thesePoints.Count]),
                            otherPointStates = new List<int>(new int[otherPoints.Count]);

                int currentIncrement = newIDS[thesePoints.Count - 1], nextIncrement;

                for (int i = thesePointStates.Count - 2; i >= 0; i--)
                {
                    Point2 thisPoint = thesePoints[i];
                    SplineSection thisSection = theseSections[i];

                    nextIncrement = newIDS[i];

                    float totalSecWeight = thisSection.weight;

                    if (other.SharesPoint(thisPoint.point))
                    {
                        thesePointStates[i] = 0;
                    }
                    else if (other.ContainsPoint(thisPoint.point))
                    {
                        thesePointStates[i] = 1;
                    }

                    if (!thisPoint.useForeHandle)
                    {
                        int fh = i + 1;
                        if (fh >= Length - 1) { fh = 0; }


                        thisPoint.foreHandle = points[fh].point - points[i].point;

                    }
                    thisPoint.useForeHandle = true;

                    if (!thisPoint.useBackHandle)
                    {
                        int bh = i - 1;
                        if (bh < 0) { bh = Length - 2; }
                        thisPoint.backHandle = points[bh].point - points[i].point;
                    }
                    thisPoint.useBackHandle = true;

                    thesePoints[i] = thisPoint;

                    float perSoFar = 0f;
                    for (int cI = currentIncrement - 1; cI >= nextIncrement; cI--)
                    {
                        int segCounter = 1;

                        for (int j = 0; j < intersects.Length; j++)
                        {
                            if (lineSegs[j * 4] == cI)
                            {
                                Point2 iPoint = new Point2();
                                iPoint.point = intersects[j];
                                iPoint.useBackHandle = iPoint.useForeHandle = true;
                                iPoint.backHandle = SmoothedPoints[lineSegs[j * 4]] - intersects[j];
                                iPoint.foreHandle = SmoothedPoints[lineSegs[j * 4 + 1]] - intersects[j];

                                thesePoints.Insert(i + segCounter, iPoint);
                                thesePointStates.Insert(i + segCounter, (j + 1) * -1);

                                SplineSection iSection = new SplineSection(), pSection = theseSections[i + segCounter - 1];
                                iSection.curveType = sections[i].curveType;

                                float currentPercent = intersectPercents[j * 2];
                                float perDif = currentPercent - perSoFar;

                                pSection.weight = perDif * totalSecWeight;
                                iSection.weight = totalWeight - (currentPercent * totalWeight);
                                theseSections[i + segCounter - 1] = pSection;
                                theseSections.Insert(i + segCounter, iSection);

                                perSoFar = currentPercent;
                                segCounter++;
                            }
                        }
                    }
                    currentIncrement = nextIncrement;
                }
                other.SequenceIntersects(ref lineSegs, ref intersects, ref intersectPercents, false);

                currentIncrement = other.newIDS[otherPoints.Count - 1];
                for (int i = otherPointStates.Count - 2; i >= 0; i--)
                {
                    Point2 otherPoint = otherPoints[i];
                    SplineSection otherSection = otherSections[i];

                    nextIncrement = other.newIDS[i];
                    float totalSecWeight = otherSection.weight;
                    if (SharesPoint(otherPoint.point))
                    {
                        otherPointStates[i] = 1;
                    }
                    else if (ContainsPoint(otherPoint.point))
                    {
                        otherPointStates[i] = 1;
                    }
                    if (!otherPoint.useForeHandle)
                    {
                        int fh = i + 1;
                        if (fh >= other.Length - 1) { fh = 0; }
                        otherPoint.foreHandle = other.points[fh].point - other.points[i].point;
                    }
                    otherPoint.useForeHandle = true;

                    if (!otherPoint.useBackHandle)
                    {
                        int bh = i - 1;
                        if (bh < 0) { bh = other.Length - 2; }
                        otherPoint.backHandle = other.points[bh].point - other.points[i].point;
                    }
                    otherPoint.useBackHandle = true;

                    otherPoints[i] = otherPoint;

                    float perSoFar = 0f;
                    for (int cI = currentIncrement - 1; cI >= nextIncrement; cI--)
                    {
                        int segCounter = 1;
                        for (int j = 0; j < intersects.Length; j++)
                        {
                            if (lineSegs[j * 4 + 2] == cI)
                            {
                                Point2 oPoint = new Point2();
                                oPoint.point = intersects[j];
                                oPoint.useBackHandle = oPoint.useForeHandle = true;
                                oPoint.backHandle = other.SmoothedPoints[lineSegs[j * 4 + 2]] - intersects[j];
                                oPoint.foreHandle = other.SmoothedPoints[lineSegs[j * 4 + 3]] - intersects[j];

                                otherPoints.Insert(i + segCounter, oPoint);

                                for (int k = thesePointStates.Count - 1; k >= 0; k--)
                                {
                                    if (thesePoints[k].point.Equals(intersects[j]) && thesePointStates[k] < 0)
                                    {
                                        otherPointStates.Insert(i + segCounter, thesePointStates[k]);
                                        break;
                                    }
                                }

                                float currentPercent = intersectPercents[j * 2 + 1];
                                float perDif = currentPercent - perSoFar;

                                SplineSection oSection = new SplineSection(), pSection = otherSections[i + segCounter - 1];
                                oSection.curveType = other.sections[i].curveType;
                                oSection.weight = totalWeight - (currentPercent * totalWeight);

                                pSection.weight = perDif * totalSecWeight;
                                otherSections[i + segCounter - 1] = pSection;

                                otherSections.Insert(i + segCounter, oSection);

                                perSoFar = currentPercent;
                                segCounter++;
                            }
                        }
                    }
                    currentIncrement = nextIncrement;
                }

                bool allUsed = false;
                int isAllUsedStuck = 0;

                while (!allUsed)
                {
                    isAllUsedStuck++;
                    allUsed = true;
                    if (isAllUsedStuck > 25) { break; }
                    int starting = -1, current = -1;
                    for (int i = 0; i < thesePoints.Count - 1; i++)
                    {
                        if (thesePointStates[i] == 0)
                        {
                            starting = i; thesePointStates[i] = 3;
                            allUsed = false;
                            break;
                        }
                    }

                    if (allUsed) { break; }
                    // 1. Go forward through whichever list we're on as long as pointState >= 0. We set these to state 3.
                    // 2. When it's less than 0, find the same number on the OTHER list and continue forward.
                    List<Point2> currentPolyAnchors = new List<Point2>();
                    List<SplineSection> currentPolySections = new List<SplineSection>();

                    currentPolyAnchors.Add(thesePoints[starting]);

                    currentPolySections.Add(theseSections[starting]);

                    bool whichList = false, afterSwitch = false;
                    current = starting + 1;

                    if (current >= thesePoints.Count - 1) { current = 0; }

                    int isListMarchStuck = 0;
                    //intersectLog += "Starting at " + starting + nl;
                    while (current != starting || whichList)
                    {
                        if (isListMarchStuck > 75)
                        {
                            current = starting; whichList = false;
                            break;
                        }
                        isListMarchStuck++;
                        if (!whichList)
                        {
                            if (thesePointStates[current] < 0)
                            {

                                whichList = true;
                                afterSwitch = true;

                                for (int j = otherPoints.Count - 1; j >= 0; j--)
                                {
                                    if (otherPointStates[j] == thesePointStates[current])
                                    {
                                        current = j;
                                        break;
                                    }
                                }

                                // New:
                                currentPolyAnchors.Add(otherPoints[current]);

                                currentPolySections.Add(otherSections[current]);
                            }
                            else
                            {
                                if (thesePointStates[current] >= 0 && thesePointStates[current] != 1)
                                {
                                    currentPolyAnchors.Add(thesePoints[current]);

                                    currentPolySections.Add(theseSections[current]);

                                    thesePointStates[current] = 3;
                                }
                            }
                            current++;
                            if (!afterSwitch && current >= thesePoints.Count - 1) { current = 0; }
                            else if (afterSwitch) { if (current == otherPoints.Count - 1) { current = 0; } afterSwitch = false; }
                        }
                        else
                        {
                            if (otherPointStates[current] < 0)
                            {
                                whichList = !whichList;
                                afterSwitch = true;


                                for (int j = thesePoints.Count - 1; j >= 0; j--)
                                {
                                    if (thesePointStates[j] == otherPointStates[current])
                                    {

                                        current = j;
                                        break;
                                    }
                                }
                                // NEW:
                                currentPolyAnchors.Add(thesePoints[current]);

                                currentPolySections.Add(theseSections[current]);
                            }
                            else
                            {
                                if (otherPointStates[current] >= 0 && otherPointStates[current] != 1)
                                {
                                    currentPolyAnchors.Add(otherPoints[current]);

                                    currentPolySections.Add(otherSections[current]);

                                    otherPointStates[current] = 3;

                                }

                            }
                            current++;
                            if (!afterSwitch && current >= otherPoints.Count - 1) { current = 0; }
                            else if (afterSwitch) { if (current >= thesePoints.Count - 1) { current = 0; } afterSwitch = false; }

                        }
                    }

                    Spline2 p = new Spline2();
                    p.name = name + final.Count;

                    p.SetAnchorPoints(currentPolyAnchors.ToArray());
                    p.sections = currentPolySections.ToArray();
                    p.GetTotalWeight();
                    final.Add(p);
                }
            }
            return final.ToArray();
        }

        public Spline2[] SubtractPolygons(Spline2 other, int curveSampling = 5)
        {

            List<Spline2> final = new List<Spline2>();

            // Check for intersection. If they don't intersect at all, return just this one.

            int[] lineSegs;
            Float2[] intersects;
            float[] intersectPercents;
            GetIntersections(other, out lineSegs, out intersects, out intersectPercents);

            if (intersects.Length == 0)
            {
                // Debug.Log("No Intersections");
                // If they intersect, check to see if one entirely contains the other. 
                if (ContainsPoint(other.points[0].point))
                {
                    // Debug.Log("A contains B.");
                    // This Spline2 entirely contains the other. Therefore, we need to split this one in two.
                    // We start by generating two splits. TopMost - favoring Top Left over Top Right, 
                    // and BottomMost - favoring Bottom Right over Bottom Left.
                    int thisTopID = top, otherTopID = other.top,
                        otherBottomID = other.bottom, thisBottomID = bottom;
                    bool noCollisions = false;
                    bool topClear = false;
                    int loopCounter = Length * 2 + other.Length * 2;

                    while (!noCollisions)
                    {
                        if (loopCounter == 0)
                        {
                            return new Spline2[0];
                        }

                        if (!topClear)
                        {
                            if (LineIntersects(points[thisTopID].point, other.points[otherTopID].point))
                            {
                                // Move this top right by one. 
                                thisTopID++;
                                if (thisTopID >= Length)
                                {
                                    thisTopID = 0;
                                }
                                // If still no, move other top right by one.
                                if (LineIntersects(points[thisTopID].point, other.points[otherTopID].point))
                                {
                                    otherTopID++;
                                    if (otherTopID >= other.Length)
                                    {
                                        otherTopID = 0;
                                    }
                                    // If still no, move this top left by one.
                                    if (LineIntersects(points[thisTopID].point, other.points[otherTopID].point))
                                    {
                                        thisTopID--;
                                        if (thisTopID < 0)
                                        {
                                            thisTopID = Length - 1;
                                        }
                                        // If still no, move this top right by two and other by one. Resolve for this loop.
                                        if (LineIntersects(points[thisTopID].point, other.points[otherTopID].point))
                                        {
                                            thisTopID += 2;
                                            if (thisTopID >= Length)
                                            {
                                                thisTopID -= Length;
                                            }
                                            otherTopID++;
                                            if (otherTopID >= other.Length)
                                            {
                                                otherTopID = 0;
                                            }
                                        }
                                        else
                                        {
                                            topClear = true;
                                        }

                                    }
                                    else
                                    {
                                        topClear = true;
                                    }
                                }
                                else
                                {
                                    topClear = true;
                                }

                            }
                            else
                            {
                                topClear = true;
                            }
                        }

                        if (topClear)
                        {
                            if (LineIntersects(points[thisBottomID].point, other.points[otherBottomID].point))
                            {
                                // Move this top right by one. 
                                thisBottomID++;
                                if (thisBottomID >= Length)
                                {
                                    thisBottomID = 0;
                                }
                                // If still no, move other top right by one.
                                if (LineIntersects(points[thisBottomID].point, other.points[otherBottomID].point))
                                {
                                    otherBottomID++;
                                    if (otherBottomID >= other.Length)
                                    {
                                        otherBottomID = 0;
                                    }
                                    // If still no, move this top left by one.
                                    if (LineIntersects(points[thisBottomID].point, other.points[otherBottomID].point))
                                    {
                                        thisBottomID--;
                                        if (thisBottomID < 0)
                                        {
                                            thisBottomID = Length - 1;
                                        }
                                        // If still no, move this top right by two and other by one. Resolve for this loop.
                                        if (LineIntersects(points[thisBottomID].point, other.points[otherBottomID].point))
                                        {
                                            thisBottomID += 2;
                                            if (thisBottomID >= Length)
                                            {
                                                thisBottomID -= Length;
                                            }
                                            otherBottomID++;
                                            if (otherBottomID >= other.Length)
                                            {
                                                otherBottomID = 0;
                                            }
                                        }
                                        else
                                        {
                                            noCollisions = true;
                                        }

                                    }
                                    else
                                    {
                                        noCollisions = true;
                                    }
                                }
                                else
                                {
                                    noCollisions = true;
                                }

                            }
                            else
                            {
                                noCollisions = true;
                            }
                        }
                    }

                    Point2[] otherPoints = other.points, thesePoints = points;

                    //Finally, we make two shapes, by cutting along those splits and following the edges of the hole.


                    List<Point2> poly = new List<Point2>();
                    List<SplineSection> polySecs = new List<SplineSection>();

                    Point2 polyPoint = thesePoints[thisTopID];
                    SplineSection polySection;

                    Float2 otherFore, otherBack;
                    // We start at thisTopID, then jump to otherTopID.
                    // We add each point we come across, going through the whole list backward until we find otherBottomID.
                    if (!polyPoint.useForeHandle)
                    {
                        int fh = thisTopID + 1;
                        if (fh >= thesePoints.Length - 1) { fh = 0; }
                        polyPoint.foreHandle = thesePoints[fh].point - thesePoints[thisTopID].point;
                    }
                    polyPoint.useForeHandle = true;

                    if (!polyPoint.useBackHandle)
                    {
                        int bh = thisTopID - 1;
                        if (bh < 0) { bh = thesePoints.Length - 2; }
                        polyPoint.backHandle = thesePoints[bh].point - thesePoints[thisTopID].point;
                    }
                    polyPoint.useBackHandle = true;

                    poly.Add(polyPoint);

                    polySection = new SplineSection();
                    polySection.curveType = CurveType.Linear;
                    polySection.weight = defaultWeight;
                    polySecs.Add(polySection);

                    int current = otherTopID;
                    while (current != otherBottomID)
                    {
                        polyPoint = otherPoints[current];

                        if (!polyPoint.useForeHandle)
                        {
                            int fh = current + 1;
                            if (fh >= otherPoints.Length - 1) { fh = 0; }
                            otherBack = otherPoints[fh].point - otherPoints[current].point;
                        }
                        else
                        {
                            otherBack = polyPoint.foreHandle;
                        }
                        polyPoint.useForeHandle = true;

                        if (!polyPoint.useBackHandle)
                        {
                            int bh = current - 1;
                            if (bh < 0) { bh = otherPoints.Length - 2; }
                            otherFore = otherPoints[bh].point - otherPoints[current].point;
                        }
                        else
                        {
                            otherFore = polyPoint.backHandle;
                        }
                        polyPoint.useBackHandle = true;

                        polyPoint.foreHandle = otherFore;
                        polyPoint.backHandle = otherBack;
                        poly.Add(polyPoint);
                        //I'm subtracting from current here on the basis that 
                        // we're going backward through these points. The section we need should
                        // be the one BEFORE this point.
                        current--;
                        if (current < 0)
                        {
                            current = otherPoints.Length - 2;

                        }
                        polySection = other.sections[current];
                        polySecs.Add(polySection);
                    }

                    //Then we add our other bottom with a straight section
                    polyPoint = otherPoints[otherBottomID];

                    if (!polyPoint.useForeHandle)
                    {
                        int fh = otherBottomID + 1;
                        if (fh >= otherPoints.Length - 1) { fh = 0; }
                        otherBack = otherPoints[fh].point - otherPoints[otherBottomID].point;
                    }
                    else
                    {
                        otherBack = polyPoint.foreHandle;
                    }

                    polyPoint.useForeHandle = true;

                    if (!polyPoint.useBackHandle)
                    {
                        int bh = otherBottomID - 1;
                        if (bh < 0) { bh = otherPoints.Length - 2; }
                        otherFore = otherPoints[bh].point - otherPoints[otherBottomID].point;
                    }
                    else
                    {
                        otherFore = polyPoint.backHandle;
                    }
                    polyPoint.useBackHandle = true;

                    polyPoint.foreHandle = otherFore;
                    polyPoint.backHandle = otherBack;
                    poly.Add(polyPoint);

                    polySection = new SplineSection();
                    polySection.curveType = CurveType.Linear;
                    polySection.weight = other.defaultWeight;
                    polySecs.Add(polySection);

                    //Now we jump to thisBottomID and go forward until we get back to it, not adding it again.

                    current = thisBottomID;
                    while (current != thisTopID)
                    {
                        polyPoint = thesePoints[current];

                        if (!polyPoint.useForeHandle)
                        {
                            int fh = current + 1;
                            if (fh >= thesePoints.Length - 1) { fh = 0; }
                            polyPoint.foreHandle = thesePoints[fh].point - thesePoints[current].point;
                        }
                        polyPoint.useForeHandle = true;

                        if (!polyPoint.useBackHandle)
                        {
                            int bh = current - 1;
                            if (bh < 0) { bh = thesePoints.Length - 2; }
                            polyPoint.backHandle = thesePoints[bh].point - thesePoints[current].point;
                        }
                        polyPoint.useBackHandle = true;

                        poly.Add(polyPoint);

                        polySection = sections[current];
                        polySecs.Add(polySection);

                        current++;
                        if (current >= thesePoints.Length - 1) { current = 0; }
                    }

                    Spline2 p = new Spline2();
                    p.name = name + " (A)";
                    p.SetAnchorPoints(poly.ToArray());
                    p.sections = polySecs.ToArray();
                    p.GetTotalWeight();
                    final.Add(p);


                    //With our first polygon created, we clear our lists and 
                    // begin again on the second half of our hole punched poly.

                    poly.Clear();
                    polySecs.Clear();

                    polyPoint = otherPoints[otherTopID];
                    if (polyPoint.useForeHandle)
                    { otherBack = polyPoint.foreHandle; }
                    else
                    {
                        int fh = otherTopID + 1;
                        if (fh >= otherPoints.Length - 1) { fh = 0; }
                        otherBack = otherPoints[fh].point - otherPoints[otherTopID].point;
                    }
                    polyPoint.useForeHandle = true;

                    if (polyPoint.useBackHandle)
                    { otherFore = polyPoint.backHandle; }
                    else
                    {
                        int bh = otherTopID - 1;
                        if (bh < 0) { bh = otherPoints.Length - 2; }

                        otherFore = otherPoints[bh].point - otherPoints[otherTopID].point;
                    }
                    polyPoint.useBackHandle = true;

                    polyPoint.foreHandle = otherFore;
                    polyPoint.backHandle = otherBack;
                    poly.Add(polyPoint);

                    polySection = new SplineSection();
                    polySection.curveType = CurveType.Linear;
                    polySection.weight = other.defaultWeight;

                    current = thisTopID;
                    while (current != thisBottomID)
                    {
                        polyPoint = thesePoints[current];
                        if (!polyPoint.useForeHandle)
                        {
                            int fh = current + 1;
                            if (fh >= thesePoints.Length - 1) { fh = 0; }
                            polyPoint.foreHandle = thesePoints[fh].point - thesePoints[current].point;
                        }
                        polyPoint.useForeHandle = true;

                        if (!polyPoint.useBackHandle)
                        {
                            int bh = current - 1;
                            if (bh < 0) { bh = thesePoints.Length - 2; }
                            polyPoint.backHandle = thesePoints[bh].point - thesePoints[current].point;
                        }
                        polyPoint.useBackHandle = true;

                        poly.Add(polyPoint);

                        polySecs.Add(sections[current]);

                        current++;
                        if (current >= thesePoints.Length - 1) { current = 0; }
                    }

                    polyPoint = thesePoints[thisBottomID];
                    if (!polyPoint.useForeHandle)
                    {
                        int fh = thisBottomID + 1;
                        if (fh >= thesePoints.Length - 1) { fh = 0; }
                        polyPoint.foreHandle = thesePoints[fh].point - thesePoints[thisBottomID].point;
                    }
                    polyPoint.useForeHandle = true;

                    if (!polyPoint.useBackHandle)
                    {
                        int bh = thisBottomID - 1;
                        if (bh < 0) { bh = thesePoints.Length - 2; }
                        polyPoint.backHandle = thesePoints[bh].point - thesePoints[thisBottomID].point;
                    }
                    polyPoint.useBackHandle = true;

                    poly.Add(polyPoint);
                    polySection = new SplineSection();
                    polySection.curveType = CurveType.Linear;

                    polySecs.Add(polySection);

                    current = otherBottomID;
                    while (current != otherTopID)
                    {
                        polyPoint = otherPoints[current];


                        if (polyPoint.useForeHandle)
                        { otherBack = polyPoint.foreHandle; }
                        else
                        {
                            int fh = current + 1;
                            if (fh >= otherPoints.Length - 1) { fh = 0; }
                            otherBack = otherPoints[fh].point - otherPoints[current].point;
                        }
                        polyPoint.useForeHandle = true;

                        if (polyPoint.useBackHandle)
                        { otherFore = polyPoint.backHandle; }
                        else
                        {
                            int bh = current - 1;
                            if (bh < 0) { bh = otherPoints.Length - 2; }

                            otherFore = otherPoints[bh].point - otherPoints[current].point;
                        }
                        polyPoint.useBackHandle = true;
                        polyPoint.foreHandle = otherFore;
                        polyPoint.backHandle = otherBack;
                        poly.Add(polyPoint);
                        polySection = other.sections[current];
                        polySecs.Add(polySection);

                        current--;
                        if (current < 0)
                        {
                            current = otherPoints.Length - 2;

                        }
                    }
                    p = new Spline2();
                    p.name = name + " (B)";

                    p.SetAnchorPoints(poly.ToArray());
                    p.sections = polySecs.ToArray();
                    p.GetTotalWeight();
                    final.Add(p);
                }
                else if (other.ContainsPoint(this[0]))
                {
                    // Debug.Log("B Contains A. Return Nothing!!");
                    //The other one entirely contains this one. Send back nothing.
                    return new Spline2[0];
                }
                else
                {
                    // Debug.Log("B subtracts nothing from A. Returning A intact.");
                    //They don't touch at all. Send back this, as is.
                    final.Add(this);
                }
            }
            else
            {
                List<Point2> thesePoints = new List<Point2>(points), otherPoints = new List<Point2>(other.points);

                List<SplineSection> theseSections = new List<SplineSection>(sections),
                                  otherSections = new List<SplineSection>(other.sections);

                List<int> thesePointStates = new List<int>(new int[thesePoints.Count]),
                            otherPointStates = new List<int>(new int[otherPoints.Count]);

                int currentIncrement = newIDS[thesePoints.Count - 1], nextIncrement;

                for (int i = thesePointStates.Count - 2; i >= 0; i--)
                {
                    Point2 thisPoint = thesePoints[i];
                    SplineSection thisSection = theseSections[i];

                    nextIncrement = newIDS[i];

                    float totalSecWeight = thisSection.weight;

                    if (other.SharesPoint(thisPoint.point))
                    {
                        thesePointStates[i] = 0;
                    }
                    else if (other.ContainsPoint(thisPoint.point))
                    {
                        thesePointStates[i] = 1;
                    }

                    if (!thisPoint.useForeHandle)
                    {
                        int fh = i + 1;
                        if (fh >= Length - 1) { fh = 0; }


                        thisPoint.foreHandle = points[fh].point - points[i].point;

                    }
                    thisPoint.useForeHandle = true;

                    if (!thisPoint.useBackHandle)
                    {
                        int bh = i - 1;
                        if (bh < 0) { bh = Length - 2; }
                        thisPoint.backHandle = points[bh].point - points[i].point;
                    }
                    thisPoint.useBackHandle = true;

                    thesePoints[i] = thisPoint;

                    float perSoFar = 0f;
                    for (int cI = currentIncrement - 1; cI >= nextIncrement; cI--)
                    {
                        int segCounter = 1;

                        for (int j = 0; j < intersects.Length; j++)
                        {
                            if (lineSegs[j * 4] == cI)
                            {
                                Point2 iPoint = new Point2();
                                iPoint.point = intersects[j];
                                iPoint.useBackHandle = iPoint.useForeHandle = true;
                                iPoint.backHandle = SmoothedPoints[lineSegs[j * 4]] - intersects[j];
                                iPoint.foreHandle = SmoothedPoints[lineSegs[j * 4 + 1]] - intersects[j];

                                thesePoints.Insert(i + segCounter, iPoint);
                                thesePointStates.Insert(i + segCounter, (j + 1) * -1);

                                SplineSection iSection = new SplineSection(), pSection = theseSections[i + segCounter - 1];
                                iSection.curveType = sections[i].curveType;

                                float currentPercent = intersectPercents[j * 2];
                                float perDif = currentPercent - perSoFar;

                                pSection.weight = perDif * totalSecWeight;
                                iSection.weight = totalWeight - (currentPercent * totalWeight);
                                theseSections[i + segCounter - 1] = pSection;
                                theseSections.Insert(i + segCounter, iSection);

                                perSoFar = currentPercent;
                                segCounter++;
                            }
                        }
                    }
                    currentIncrement = nextIncrement;


                }

                other.SequenceIntersects(ref lineSegs, ref intersects, ref intersectPercents, false);

                currentIncrement = other.newIDS[otherPoints.Count - 1];
                for (int i = otherPointStates.Count - 2; i >= 0; i--)
                {
                    Point2 otherPoint = otherPoints[i];
                    SplineSection otherSection = otherSections[i];

                    nextIncrement = other.newIDS[i];
                    float totalSecWeight = otherSection.weight;
                    if (SharesPoint(otherPoint.point))
                    {
                        otherPointStates[i] = 1;
                    }
                    else if (ContainsPoint(otherPoint.point))
                    {
                        otherPointStates[i] = 1;
                    }

                    if (!otherPoint.useForeHandle)
                    {
                        int fh = i + 1;
                        if (fh >= other.Length - 1) { fh = 0; }
                        otherPoint.foreHandle = other.points[fh].point - other.points[i].point;
                    }
                    otherPoint.useForeHandle = true;

                    if (!otherPoint.useBackHandle)
                    {
                        int bh = i - 1;
                        if (bh < 0) { bh = other.Length - 2; }
                        otherPoint.backHandle = other.points[bh].point - other.points[i].point;
                    }
                    otherPoint.useBackHandle = true;

                    otherPoints[i] = otherPoint;

                    float perSoFar = 0f;
                    for (int cI = currentIncrement - 1; cI >= nextIncrement; cI--)
                    {
                        int segCounter = 1;
                        for (int j = 0; j < intersects.Length; j++)
                        {
                            if (lineSegs[j * 4 + 2] == cI)
                            {
                                Point2 oPoint = new Point2();
                                oPoint.point = intersects[j];
                                oPoint.useBackHandle = oPoint.useForeHandle = true;
                                oPoint.backHandle = other.SmoothedPoints[lineSegs[j * 4 + 2]] - intersects[j];
                                oPoint.foreHandle = other.SmoothedPoints[lineSegs[j * 4 + 3]] - intersects[j];

                                otherPoints.Insert(i + segCounter, oPoint);

                                for (int k = thesePointStates.Count - 1; k >= 0; k--)
                                {
                                    if (thesePoints[k].point.Equals(intersects[j]) && thesePointStates[k] < 0)
                                    {
                                        otherPointStates.Insert(i + segCounter, thesePointStates[k]);
                                        break;
                                    }
                                }

                                float currentPercent = intersectPercents[j * 2 + 1];
                                float perDif = currentPercent - perSoFar;

                                SplineSection oSection = new SplineSection(), pSection = otherSections[i + segCounter - 1];
                                oSection.curveType = other.sections[i].curveType;
                                oSection.weight = totalWeight - (currentPercent * totalWeight);

                                pSection.weight = perDif * totalSecWeight;
                                otherSections[i + segCounter - 1] = pSection;

                                otherSections.Insert(i + segCounter, oSection);

                                perSoFar = currentPercent;
                                segCounter++;
                            }
                        }
                    }
                    currentIncrement = nextIncrement;


                }

                bool allUsed = false;
                int isAllUsedStuck = 0;

                while (!allUsed)
                {
                    isAllUsedStuck++;
                    allUsed = true;
                    if (isAllUsedStuck > 25) { break; }

                    int starting = -1, current = -1;
                    for (int i = 0; i < thesePoints.Count - 1; i++)
                    {
                        if (thesePointStates[i] == 0)
                        {
                            starting = i; thesePointStates[i] = 3;
                            allUsed = false;
                            break;
                        }
                    }

                    if (allUsed) { break; }
                    // 1. Go forward through whichever list we're on as long as pointState >= 0. We set these to state 3.
                    // 2. When it's less than 0, find the same number on the OTHER list and continue forward.
                    List<Point2> currentPolyAnchors = new List<Point2>();
                    List<SplineSection> currentPolySections = new List<SplineSection>();

                    Point2 polyPoint;
                    //SplineSection polySection;

                    currentPolyAnchors.Add(thesePoints[starting]);

                    currentPolySections.Add(theseSections[starting]);

                    bool whichList = false, afterSwitch = false;
                    current = starting + 1;

                    if (current >= thesePoints.Count - 1) { current = 0; }

                    int isListMarchStuck = 0;
                    //intersectLog += "Starting at " + starting + nl;
                    while (current != starting || whichList)
                    {
                        if (isListMarchStuck > 75)
                        {
                            current = starting; whichList = false;
                            break;
                        }
                        isListMarchStuck++;

                        if (!whichList)
                        {
                            if (thesePointStates[current] < 0)
                            {

                                whichList = true;
                                afterSwitch = true;

                                for (int j = otherPoints.Count - 1; j >= 0; j--)
                                {
                                    if (otherPointStates[j] == thesePointStates[current])
                                    {
                                        current = j;
                                        break;
                                    }
                                }
                                polyPoint = new Point2();

                                polyPoint.point = otherPoints[current].point;

                                polyPoint.useBackHandle = otherPoints[current].useForeHandle;
                                polyPoint.backHandle = otherPoints[current].foreHandle;
                                polyPoint.useForeHandle = otherPoints[current].useBackHandle;
                                polyPoint.foreHandle = otherPoints[current].backHandle;

                                currentPolyAnchors.Add(polyPoint);
                                int c = current - 1;
                                if (c < 0) { c = otherPoints.Count - 2; }

                                currentPolySections.Add(otherSections[c]);
                            }
                            else
                            {
                                if (thesePointStates[current] >= 0 && thesePointStates[current] != 1)
                                {
                                    currentPolyAnchors.Add(thesePoints[current]);

                                    currentPolySections.Add(theseSections[current]);

                                    thesePointStates[current] = 3;
                                }
                            }

                            if (!afterSwitch)
                            {
                                current++;
                                if (current >= thesePoints.Count - 1) { current = 0; }
                            }
                            else
                            {
                                current--;
                                if (current < 0) { current = otherPoints.Count - 2; }
                                afterSwitch = false;
                            }
                        }
                        else
                        {
                            if (otherPointStates[current] < 0)
                            {
                                whichList = !whichList;
                                afterSwitch = true;


                                for (int j = thesePoints.Count - 1; j >= 0; j--)
                                {
                                    if (thesePointStates[j] == otherPointStates[current])
                                    {

                                        current = j;
                                        break;
                                    }
                                }
                                // NEW:
                                currentPolyAnchors.Add(thesePoints[current]);

                                currentPolySections.Add(theseSections[current]);
                            }
                            else
                            {
                                if (otherPointStates[current] == 1)
                                {
                                    polyPoint = new Point2();

                                    polyPoint.point = otherPoints[current].point;

                                    polyPoint.useBackHandle = otherPoints[current].useForeHandle;
                                    polyPoint.backHandle = otherPoints[current].foreHandle;
                                    polyPoint.useForeHandle = otherPoints[current].useBackHandle;
                                    polyPoint.foreHandle = otherPoints[current].backHandle;

                                    currentPolyAnchors.Add(polyPoint);
                                    int c = current - 1;
                                    if (c < 0) { c = otherPoints.Count - 2; }

                                    currentPolySections.Add(otherSections[c]);

                                    otherPointStates[current] = 3;

                                }

                            }
                            if (!afterSwitch)
                            {
                                current--;
                                if (current < 0) { current = otherPoints.Count - 2; }
                            }
                            else
                            {
                                current++;
                                if (current >= thesePoints.Count - 1) { current = 0; }
                                afterSwitch = false;
                            }
                        }
                    }

                    Spline2 p = new Spline2();
                    p.name = name + final.Count;

                    p.SetAnchorPoints(currentPolyAnchors.ToArray());
                    p.sections = currentPolySections.ToArray();
                    p.GetTotalWeight();
                    final.Add(p);
                }


            }

            return final.ToArray();

        }

        public Spline2[] GetIntersectingPolygons(Spline2 other, int curveSampling = 5)
        {
            List<Spline2> final = new List<Spline2>();

            int[] lineSegs;
            Float2[] intersects;
            float[] intersectPercents;
            GetIntersections(other, out lineSegs, out intersects, out intersectPercents);

            if (intersects.Length == 0)
            {
                if (ContainsPoint(other.points[0].point)) { final.Add(other); }
                else if (other.ContainsPoint(this[0])) { final.Add(this); }
            }
            else
            {
                List<Point2> thesePoints = new List<Point2>(points), otherPoints = new List<Point2>(other.points);

                List<SplineSection> theseSections = new List<SplineSection>(sections),
                                  otherSections = new List<SplineSection>(other.sections);

                List<int> thesePointStates = new List<int>(new int[thesePoints.Count]),
                            otherPointStates = new List<int>(new int[otherPoints.Count]);

                int currentIncrement = newIDS[thesePoints.Count - 1], nextIncrement;

                for (int i = thesePointStates.Count - 2; i >= 0; i--)
                {
                    Point2 thisPoint = thesePoints[i];
                    SplineSection thisSection = theseSections[i];

                    nextIncrement = newIDS[i];

                    float totalSecWeight = thisSection.weight;

                    if (other.ContainsPoint(thisPoint.point))
                    {
                        thesePointStates[i] = 1;
                    }

                    if (!thisPoint.useForeHandle)
                    {
                        int fh = i + 1;
                        if (fh >= Length - 1) { fh = 0; }

                        thisPoint.foreHandle = points[fh].point - points[i].point;

                    }
                    thisPoint.useForeHandle = true;

                    if (!thisPoint.useBackHandle)
                    {
                        int bh = i - 1;
                        if (bh < 0) { bh = Length - 2; }
                        thisPoint.backHandle = points[bh].point - points[i].point;
                    }
                    thisPoint.useBackHandle = true;

                    thesePoints[i] = thisPoint;

                    float perSoFar = 0f;
                    for (int cI = currentIncrement - 1; cI >= nextIncrement; cI--)
                    {
                        int segCounter = 1;

                        for (int j = 0; j < intersects.Length; j++)
                        {
                            if (lineSegs[j * 4] == cI)
                            {
                                Point2 iPoint = new Point2();
                                iPoint.point = intersects[j];
                                iPoint.useBackHandle = iPoint.useForeHandle = true;
                                iPoint.backHandle = SmoothedPoints[lineSegs[j * 4]] - intersects[j];
                                iPoint.foreHandle = SmoothedPoints[lineSegs[j * 4 + 1]] - intersects[j];

                                thesePoints.Insert(i + segCounter, iPoint);
                                thesePointStates.Insert(i + segCounter, (j + 1) * -1);

                                SplineSection iSection = new SplineSection(), pSection = theseSections[i + segCounter - 1];
                                iSection.curveType = sections[i].curveType;

                                float currentPercent = intersectPercents[j * 2];
                                float perDif = currentPercent - perSoFar;

                                pSection.weight = perDif * totalSecWeight;
                                iSection.weight = totalWeight - (currentPercent * totalWeight);
                                theseSections[i + segCounter - 1] = pSection;
                                theseSections.Insert(i + segCounter, iSection);

                                perSoFar = currentPercent;
                                segCounter++;
                            }
                        }
                    }
                    currentIncrement = nextIncrement;
                }
                other.SequenceIntersects(ref lineSegs, ref intersects, ref intersectPercents, false);

                currentIncrement = other.newIDS[otherPoints.Count - 1];
                for (int i = otherPointStates.Count - 2; i >= 0; i--)
                {
                    Point2 otherPoint = otherPoints[i];
                    SplineSection otherSection = otherSections[i];

                    nextIncrement = other.newIDS[i];
                    float totalSecWeight = otherSection.weight;
                    if (ContainsPoint(otherPoint.point))
                    {
                        otherPointStates[i] = 1;
                    }

                    if (!otherPoint.useForeHandle)
                    {
                        int fh = i + 1;
                        if (fh >= other.Length - 1) { fh = 0; }
                        otherPoint.foreHandle = other.points[fh].point - other.points[i].point;
                    }
                    otherPoint.useForeHandle = true;

                    if (!otherPoint.useBackHandle)
                    {
                        int bh = i - 1;
                        if (bh < 0) { bh = other.Length - 2; }
                        otherPoint.backHandle = other.points[bh].point - other.points[i].point;
                    }
                    otherPoint.useBackHandle = true;

                    otherPoints[i] = otherPoint;

                    float perSoFar = 0f;
                    for (int cI = currentIncrement - 1; cI >= nextIncrement; cI--)
                    {
                        int segCounter = 1;
                        for (int j = 0; j < intersects.Length; j++)
                        {
                            if (lineSegs[j * 4 + 2] == cI)
                            {
                                Point2 oPoint = new Point2();
                                oPoint.point = intersects[j];
                                oPoint.useBackHandle = oPoint.useForeHandle = true;
                                oPoint.backHandle = other.SmoothedPoints[lineSegs[j * 4 + 2]] - intersects[j];
                                oPoint.foreHandle = other.SmoothedPoints[lineSegs[j * 4 + 3]] - intersects[j];

                                otherPoints.Insert(i + segCounter, oPoint);

                                for (int k = thesePointStates.Count - 1; k >= 0; k--)
                                {
                                    if (thesePoints[k].point.Equals(intersects[j]))
                                    {
                                        otherPointStates.Insert(i + segCounter, thesePointStates[k]);
                                        break;
                                    }
                                }

                                float currentPercent = intersectPercents[j * 2 + 1];
                                float perDif = currentPercent - perSoFar;

                                SplineSection oSection = new SplineSection(), pSection = otherSections[i + segCounter - 1];
                                oSection.curveType = other.sections[i].curveType;
                                oSection.weight = totalWeight - (currentPercent * totalWeight);

                                pSection.weight = perDif * totalSecWeight;
                                otherSections[i + segCounter - 1] = pSection;

                                otherSections.Insert(i + segCounter, oSection);

                                perSoFar = currentPercent;
                                segCounter++;
                            }
                        }
                    }
                    currentIncrement = nextIncrement;
                }

                bool allUsed = false;
                int isAllUsedStuck = 0;

                while (!allUsed)
                {
                    isAllUsedStuck++;
                    allUsed = true;

                    if (isAllUsedStuck > 25) {  break; }

                    int starting = -1, current = -1;

                    bool startList = false;

                    for (int i = 0; i < thesePoints.Count - 1; i++)
                    {
                        if (thesePointStates[i] == 1)
                        {
                            starting = i; thesePointStates[i] = 3;
                            allUsed = false;
                            break;
                        }
                    }

                    if (allUsed)
                    {
                        for (int i = 0; i < otherPoints.Count; i++)
                        {
                            if (otherPointStates[i] == 1)
                            {
                                starting = i; otherPointStates[i] = 3;
                                startList = true;
                                allUsed = false;
                                break;
                            }
                        }
                        if (allUsed) { break; }
                    }

                    List<Point2> currentPolyAnchors = new List<Point2>();
                    List<SplineSection> currentPolySections = new List<SplineSection>();

                    if (!startList)
                    {
                        currentPolyAnchors.Add(thesePoints[starting]);
                        currentPolySections.Add(sections[starting]);
                    }
                    else
                    {
                        currentPolyAnchors.Add(otherPoints[starting]);
                        currentPolySections.Add(other.sections[starting]);
                    }

                    bool whichList = startList, afterSwitch = false;
                    current = starting + 1;

                    int isListMarchStuck = 0;
                    //intersectLog += "Starting at " + starting + nl;
                    while (current != starting || whichList != startList)
                    {
                        if (isListMarchStuck > 75)
                        {
                            current = starting; whichList = false;
                            break;
                        }
                        isListMarchStuck++;

                        if (!whichList)
                        {
                            if (thesePointStates[current] < 0)
                            {

                                whichList = true;
                                afterSwitch = true;

                                for (int j = otherPoints.Count - 1; j >= 0; j--)
                                {
                                    if (otherPointStates[j] == thesePointStates[current])
                                    {
                                        current = j;
                                        break;
                                    }
                                }

                                currentPolyAnchors.Add(otherPoints[current]);
                                currentPolySections.Add(otherSections[current]);
                            }
                            else
                            {
                                if (thesePointStates[current] == 1)
                                {
                                    currentPolyAnchors.Add(thesePoints[current]);

                                    currentPolySections.Add(theseSections[current]);

                                    thesePointStates[current] = 3;
                                }
                            }
                            current++;
                            if (!afterSwitch && current == thesePoints.Count - 1) { current = 0; }
                            else if (afterSwitch) { if (current == otherPoints.Count - 1) { current = 0; } afterSwitch = false; }
                        }
                        else
                        {
                            if (otherPointStates[current] < 0)
                            {
                                whichList = false;
                                afterSwitch = true;


                                for (int j = thesePoints.Count - 1; j >= 0; j--)
                                {
                                    if (thesePointStates[j] == otherPointStates[current])
                                    {

                                        current = j;
                                        break;
                                    }
                                }
                                // NEW:
                                currentPolyAnchors.Add(thesePoints[current]);

                                currentPolySections.Add(theseSections[current]);
                            }
                            else
                            {
                                if (otherPointStates[current] == 1)
                                {
                                    currentPolyAnchors.Add(otherPoints[current]);

                                    currentPolySections.Add(otherSections[current]);

                                    otherPointStates[current] = 3;

                                }

                            }
                            current++;
                            if (!afterSwitch && current == otherPoints.Count - 1) { current = 0; }
                            else if (afterSwitch) { if (current == thesePoints.Count - 1) { current = 0; } afterSwitch = false; }
                        }
                    }

                    Spline2 p = new Spline2();
                    p.name = name + final.Count;

                    p.SetAnchorPoints(currentPolyAnchors.ToArray());
                    p.sections = currentPolySections.ToArray();
                    p.GetTotalWeight();
                    final.Add(p);
                }

            }
            return final.ToArray();
        }

        public Spline2[] GetNonIntersectingPolygons(Spline2 other, int curveSampling = 5)
        {
            List<Spline2> final = new List<Spline2>();

            // Check for intersection. If they don't intersect at all, return just this one.

            int[] lineSegs;
            Float2[] intersects;
            float[] intersectPercents;
            GetIntersections(other, out lineSegs, out intersects, out intersectPercents);

            if (intersects.Length == 0)
            {
                if (ContainsPoint(other[0]))
                {
                    return SubtractPolygons(other);
                }
                else if (other.ContainsPoint(this[0]))
                {
                    return other.SubtractPolygons(this);
                }
                else
                {
                    //They don't touch at all. Send back both as is.
                    final.Add(this);
                    final.Add(other);
                }
            }
            else
            {
                List<Point2> thesePoints = new List<Point2>(points), otherPoints = new List<Point2>(other.points);

                List<SplineSection> theseSections = new List<SplineSection>(sections),
                                  otherSections = new List<SplineSection>(other.sections);

                List<int> thesePointStates = new List<int>(new int[thesePoints.Count]),
                            otherPointStates = new List<int>(new int[otherPoints.Count]);

                int currentIncrement = newIDS[thesePoints.Count - 1], nextIncrement;

                for (int i = thesePointStates.Count - 2; i >= 0; i--)
                {
                    Point2 thisPoint = thesePoints[i];
                    SplineSection thisSection = theseSections[i];

                    nextIncrement = newIDS[i];

                    float totalSecWeight = thisSection.weight;

                    if (other.ContainsPoint(thisPoint.point))
                    {
                        thesePointStates[i] = 1;
                    }

                    if (!thisPoint.useForeHandle)
                    {
                        int fh = i + 1;
                        if (fh >= Length - 1) { fh = 0; }

                        thisPoint.foreHandle = points[fh].point - points[i].point;

                    }
                    thisPoint.useForeHandle = true;

                    if (!thisPoint.useBackHandle)
                    {
                        int bh = i - 1;
                        if (bh < 0) { bh = Length - 2; }
                        thisPoint.backHandle = points[bh].point - points[i].point;
                    }
                    thisPoint.useBackHandle = true;

                    thesePoints[i] = thisPoint;

                    float perSoFar = 0f;
                    for (int cI = currentIncrement - 1; cI >= nextIncrement; cI--)
                    {
                        int segCounter = 1;

                        for (int j = 0; j < intersects.Length; j++)
                        {
                            if (lineSegs[j * 4] == cI)
                            {
                                Point2 iPoint = new Point2();
                                iPoint.point = intersects[j];
                                iPoint.useBackHandle = iPoint.useForeHandle = true;
                                iPoint.backHandle = SmoothedPoints[lineSegs[j * 4]] - intersects[j];
                                iPoint.foreHandle = SmoothedPoints[lineSegs[j * 4 + 1]] - intersects[j];

                                thesePoints.Insert(i + segCounter, iPoint);
                                thesePointStates.Insert(i + segCounter, (j + 1) * -1);

                                SplineSection iSection = new SplineSection(), pSection = theseSections[i + segCounter - 1];
                                iSection.curveType = sections[i].curveType;

                                float currentPercent = intersectPercents[j * 2];
                                float perDif = currentPercent - perSoFar;

                                pSection.weight = perDif * totalSecWeight;
                                iSection.weight = totalWeight - (currentPercent * totalWeight);
                                theseSections[i + segCounter - 1] = pSection;
                                theseSections.Insert(i + segCounter, iSection);

                                perSoFar = currentPercent;
                                segCounter++;
                            }
                        }
                    }
                    currentIncrement = nextIncrement;
                }
                other.SequenceIntersects(ref lineSegs, ref intersects, ref intersectPercents, false);

                currentIncrement = other.newIDS[otherPoints.Count - 1];
                for (int i = otherPointStates.Count - 2; i >= 0; i--)
                {
                    Point2 otherPoint = otherPoints[i];
                    SplineSection otherSection = otherSections[i];

                    nextIncrement = other.newIDS[i];
                    float totalSecWeight = otherSection.weight;
                    if (ContainsPoint(otherPoint.point))
                    {
                        otherPointStates[i] = 1;
                    }

                    if (!otherPoint.useForeHandle)
                    {
                        int fh = i + 1;
                        if (fh >= other.Length - 1) { fh = 0; }
                        otherPoint.foreHandle = other.points[fh].point - other.points[i].point;
                    }
                    otherPoint.useForeHandle = true;

                    if (!otherPoint.useBackHandle)
                    {
                        int bh = i - 1;
                        if (bh < 0) { bh = other.Length - 2; }
                        otherPoint.backHandle = other.points[bh].point - other.points[i].point;
                    }
                    otherPoint.useBackHandle = true;

                    otherPoints[i] = otherPoint;

                    float perSoFar = 0f;
                    for (int cI = currentIncrement - 1; cI >= nextIncrement; cI--)
                    {
                        int segCounter = 1;
                        for (int j = 0; j < intersects.Length; j++)
                        {
                            if (lineSegs[j * 4 + 2] == cI)
                            {
                                Point2 oPoint = new Point2();
                                oPoint.point = intersects[j];
                                oPoint.useBackHandle = oPoint.useForeHandle = true;
                                oPoint.backHandle = other.SmoothedPoints[lineSegs[j * 4 + 2]] - intersects[j];
                                oPoint.foreHandle = other.SmoothedPoints[lineSegs[j * 4 + 3]] - intersects[j];

                                otherPoints.Insert(i + segCounter, oPoint);

                                for (int k = thesePointStates.Count - 1; k >= 0; k--)
                                {
                                    if (thesePoints[k].point.Equals(intersects[j]))
                                    {
                                        otherPointStates.Insert(i + segCounter, thesePointStates[k]);
                                        break;
                                    }
                                }

                                float currentPercent = intersectPercents[j * 2 + 1];
                                float perDif = currentPercent - perSoFar;

                                SplineSection oSection = new SplineSection(), pSection = otherSections[i + segCounter - 1];
                                oSection.curveType = other.sections[i].curveType;
                                oSection.weight = totalWeight - (currentPercent * totalWeight);

                                pSection.weight = perDif * totalSecWeight;
                                otherSections[i + segCounter - 1] = pSection;

                                otherSections.Insert(i + segCounter, oSection);

                                perSoFar = currentPercent;
                                segCounter++;
                            }
                        }
                    }
                    currentIncrement = nextIncrement;
                }

                bool allUsed = false;
                int isAllUsedStuck = 0;

                while (!allUsed)
                {
                    isAllUsedStuck++;
                    allUsed = true;

                    if (isAllUsedStuck > 25) { break; }

                    int starting = -1, current = -1;

                    bool startList = false;

                    for (int i = 0; i < thesePoints.Count - 1; i++)
                    {
                        if (thesePointStates[i] == 0)
                        {
                            starting = i; thesePointStates[i] = 3;
                            allUsed = false;
                            break;
                        }
                    }

                    if (allUsed)
                    {
                        for (int i = 0; i < otherPoints.Count; i++)
                        {
                            if (otherPointStates[i] == 0)
                            {
                                starting = i; otherPointStates[i] = 3;
                                startList = true;
                                allUsed = false;
                                break;
                            }
                        }
                        if (allUsed) { break; }
                    }

                    List<Point2> currentPolyAnchors = new List<Point2>();
                    List<SplineSection> currentPolySections = new List<SplineSection>();

                    if (!startList)
                    {
                        currentPolyAnchors.Add(thesePoints[starting]);
                        currentPolySections.Add(sections[starting]);
                    }
                    else
                    {
                        currentPolyAnchors.Add(otherPoints[starting]);
                        currentPolySections.Add(other.sections[starting]);
                    }

                    bool whichList = startList, afterSwitch = false;
                    current = starting + 1;

                    int isListMarchStuck = 0;
                    //intersectLog += "Starting at " + starting + nl;
                    while (current != starting || whichList != startList)
                    {
                        if (isListMarchStuck > 75)
                        {
                            current = starting; whichList = false;
                            break;
                        }
                        isListMarchStuck++;

                        if (!whichList)
                        {
                            if (thesePointStates[current] < 0)
                            {

                                whichList = true;
                                afterSwitch = true;

                                for (int j = otherPoints.Count - 1; j >= 0; j--)
                                {
                                    if (otherPointStates[j] == thesePointStates[current])
                                    {
                                        current = j;
                                        break;
                                    }
                                }
                                if (!startList)
                                {
                                    Point2 polyPoint = otherPoints[current];
                                    polyPoint.useForeHandle = polyPoint.useBackHandle;
                                    polyPoint.foreHandle = polyPoint.backHandle;
                                    polyPoint.backHandle = otherPoints[current].foreHandle;
                                    polyPoint.useBackHandle = otherPoints[current].useForeHandle;

                                    currentPolyAnchors.Add(polyPoint);
                                    int c = current - 1;
                                    if (c < 0) { c = otherPoints.Count - 2; }
                                    currentPolySections.Add(otherSections[c]);
                                }
                                else
                                {
                                    currentPolyAnchors.Add(otherPoints[current]);
                                    currentPolySections.Add(otherSections[current]);
                                }

                            }
                            else
                            {
                                if (!startList)
                                {
                                    currentPolyAnchors.Add(thesePoints[current]);
                                    currentPolySections.Add(theseSections[current]);
                                }
                                else
                                {
                                    Point2 polyPoint = thesePoints[current];
                                    polyPoint.useForeHandle = polyPoint.useBackHandle;
                                    polyPoint.foreHandle = polyPoint.backHandle;
                                    polyPoint.backHandle = thesePoints[current].foreHandle;
                                    polyPoint.useBackHandle = thesePoints[current].useForeHandle;

                                    currentPolyAnchors.Add(polyPoint);
                                    int c = current - 1;
                                    if (c < 0) { c = thesePoints.Count - 2; }
                                    currentPolySections.Add(theseSections[c]);
                                }
                                thesePointStates[current] = 3;

                            }
                            if (!startList)
                            {
                                if (!afterSwitch)
                                {
                                    current++;
                                    if (current == thesePoints.Count - 1) { current = 0; }
                                }
                                else
                                {
                                    current--;
                                    if (current < 0) { current = otherPoints.Count - 2; }
                                    afterSwitch = false;
                                }
                            }
                            else
                            {
                                if (!afterSwitch)
                                {
                                    current--;
                                    if (current < 0) { current = thesePoints.Count - 2; }
                                }
                                else
                                {
                                    current++;
                                    if (current == otherPoints.Count - 1) { current = 0; }
                                    afterSwitch = false;
                                }
                            }
                        }
                        else
                        {
                            if (otherPointStates[current] < 0)
                            {
                                whichList = false;
                                afterSwitch = true;

                                int oldCurrent = current;

                                for (int j = thesePoints.Count - 1; j >= 0; j--)
                                {
                                    if (thesePointStates[j] == otherPointStates[current])
                                    {

                                        current = j;
                                        break;
                                    }
                                }
                                Point2 polyPoint;
                                SplineSection polySection;
                                if (!startList)
                                {
                                    polyPoint = thesePoints[current];
                                    polyPoint.useBackHandle = otherPoints[oldCurrent].useForeHandle;
                                    polySection = theseSections[current];
                                }
                                else
                                {
                                    polyPoint = thesePoints[current];
                                    polyPoint.foreHandle = polyPoint.backHandle;
                                    polyPoint.backHandle = thesePoints[current].foreHandle;
                                    polyPoint.useForeHandle = otherPoints[oldCurrent].useBackHandle;

                                    int c = current - 1;
                                    if (c < 0) { c = thesePoints.Count - 2; }
                                    polySection = theseSections[c];
                                }
                                currentPolyAnchors.Add(polyPoint);
                                currentPolySections.Add(polySection);


                            }
                            else
                            {
                                if (startList)
                                {
                                    currentPolyAnchors.Add(otherPoints[current]);

                                    currentPolySections.Add(otherSections[current]);
                                }
                                else
                                {
                                    Point2 polyPoint = otherPoints[current];
                                    polyPoint.foreHandle = polyPoint.backHandle;
                                    polyPoint.backHandle = otherPoints[current].foreHandle;
                                    polyPoint.useForeHandle = otherPoints[current].useBackHandle;
                                    polyPoint.useBackHandle = otherPoints[current].useForeHandle;

                                    currentPolyAnchors.Add(polyPoint);
                                    int c = current - 1;
                                    if (c < 0) { c = otherPoints.Count - 2; }
                                    currentPolySections.Add(otherSections[c]);
                                }
                                otherPointStates[current] = 3;
                            }
                            if (startList)
                            {
                                if (!afterSwitch)
                                {
                                    current++;
                                    if (current == otherPoints.Count - 1) { current = 0; }
                                }
                                else
                                {
                                    current--;
                                    if (current < 0) { current = thesePoints.Count - 2; }
                                    afterSwitch = false;
                                }
                            }
                            else
                            {
                                if (!afterSwitch)
                                {
                                    current--;
                                    if (current < 0) { current = otherPoints.Count - 2; }
                                }
                                else
                                {
                                    current++;
                                    if (current == thesePoints.Count - 1) { current = 0; }
                                    afterSwitch = false;
                                }
                            }
                        }
                    }

                    Spline2 p = new Spline2();
                    p.name = name + final.Count;

                    p.SetAnchorPoints(currentPolyAnchors.ToArray());
                    p.sections = currentPolySections.ToArray();
                    p.GetTotalWeight();
                    final.Add(p);
                }


            }

            return final.ToArray();
        }

        public int[] Triangulate()
        {

            List<int> inds = new List<int>();
            Float2[] ponts = SmoothedPoints;

            int n = ponts.Length - 1;
            if (n < 3)
            {
                return inds.ToArray();

            }

            int[] V = new int[n];
            if (Area() > 0)
            {
                for (int v = 0; v < n; v++)
                    V[v] = v;
            }
            else
            {
                for (int v = 0; v < n; v++)
                    V[v] = (n - 1) - v;
            }

            int nv = n;
            int count = 2 * n;
            //Debug.Log("count = " + count + ",");

            int loopCounter = 0, snipCounter = 0;
            for (int m = 0, v = nv - 1; nv > 2;)
            {
                loopCounter++;
                if ((count--) <= 0)
                {
                    //Debug.Log(loopCounter + " loops, " + snipCounter + ". Triangulated " + inds.Count);

                    return inds.ToArray();
                }

                int u = v;
                if (nv <= u)
                    u = 0;
                v = u + 1;
                if (nv <= v)
                    v = 0;
                int w = v + 1;
                if (nv <= w)
                    w = 0;

                if (Snip(u, v, w, nv, V))
                {
                    snipCounter++;
                    int a, b, c, s, t;
                    a = V[u];
                    b = V[v];
                    c = V[w];
                    inds.Add(a);
                    inds.Add(b);
                    inds.Add(c);

                    m++;
                    for (s = v, t = v + 1; t < nv; s++, t++)
                        V[s] = V[t];
                    nv--;
                    count = 2 * nv;
                }
            }
            //Debug.Log(loopCounter + " loops, " + snipCounter + ". Triangulated " + inds.Count);
            inds.Reverse();
            return inds.ToArray();
        }

        public float Area()
        {
            Float2[] ponts = SmoothedPoints;
            int n = ponts.Length - 1;

            float A = 0.0f;
            for (int p = n - 1, q = 0; q < n; p = q++)
            {
                Float2 pval = ponts[p];
                Float2 qval = ponts[q];
                A += pval.x * qval.y - qval.x * pval.y;
            }

            return (A * 0.5f);
        }

        private bool Snip(int u, int v, int w, int n, int[] V)
        {
            int p;
            Float2[] ponts = SmoothedPoints;

            Float2 A = ponts[V[u]];
            Float2 B = ponts[V[v]];
            Float2 C = ponts[V[w]];

            if (Float.Epsilon > (((B.x - A.x) * (C.y - A.y)) - ((B.y - A.y) * (C.x - A.x))))
            {
                return false;
            }
            //Debug.Log(+ " passed epsilon test.");
            for (p = 0; p < n; p++)
            {
                if ((p == u) || (p == v) || (p == w))
                {
                    continue;
                }
                Float2 P = ponts[V[p]];
                if (Float2.InsideTriangle(A, B, C, P))
                {
                    //Debug.Log(V[p] + " is inside triangle " + V[u] + "/ " + V[v] + "/ " + V[w]);
                    return false;
                }
            }
            return true;
        }

        
        #endregion

    }

    [System.Serializable]
    public class Spline3 : SplineBase
    {
        #region Variables

        public Point3[] points = new Point3[0];


        public Bounds3 extents;
        public bool extentsDirty = true;

        public int top, smoothTop, left, smoothLeft, bottom, smoothBottom, right, smoothRight, front, smoothFront, back, smoothBack;

        public Float3[] smoothedPoints = new Float3[0];

        public Float3 startCap, endCap;

        Float3 a, b, c, d;

        Float3 min, max, smoothMin, smoothMax;

        float splineLength = -1;

        #endregion

        #region Properties
        /// <summary>
        /// The top-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Top { get { if (extentsDirty) { Bounds3 e = Extents; } return top; } }

        public int SmoothTop { get { if (extentsDirty) { Bounds3 e = Extents; } return smoothTop; } }

        /// <summary>
        /// The left-most point. If two or more points share the same min X value, the bottom-most point is used.
        /// </summary>
        public int Left { get { if (extentsDirty) { Bounds3 e = Extents; } return left; } }

        public int SmoothLeft { get { if (extentsDirty) { Bounds3 e = Extents; } return smoothLeft; } }

        /// <summary>
        /// The bottom-most point. If two or more points share the same min Y value, the right-most point is used.
        /// </summary>
        public int Bottom { get { if (extentsDirty) { Bounds3 e = Extents; } return bottom; } }

        public int SmoothBottom { get { if (extentsDirty) { Bounds3 e = Extents; } return smoothBottom; } }

        /// <summary>
        /// The right-most point. If two or more points share the same max X value, the top-most point is used.
        /// </summary>
        public int Right { get { if (extentsDirty) { Bounds3 e = Extents; } return right; } }

        public int SmoothRight { get { if (extentsDirty) { Bounds3 e = Extents; } return smoothRight; } }

        /// <summary>
        /// The front-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Front { get { if (extentsDirty) { Bounds3 e = Extents; } return front; } }

        public int SmoothFront { get { if (extentsDirty) { Bounds3 e = Extents; } return smoothFront; } }

        /// <summary>
        /// The back-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Back { get { if (extentsDirty) { Bounds3 e = Extents; } return back; } }

        public int SmoothBack { get { if (extentsDirty) { Bounds3 e = Extents; } return smoothBack; } }

        public Float3 this[int index]
        {
            get { return points[index].point; }
            set
            {
                anchorsDirty = true;
                if (wrap && (index == 0 || index == Length))
                {
                    points[0].point = points[Length].point = value;
                }
                else
                {
                    points[index].point = value;

                }
            }
        }

        public Float3[] AnchorPoints
        {
            get
            {
                Float3[] nP = new Float3[points.Length];
                for (int i = nP.Length - 1; i >= 0; i--)
                {
                    nP[i] = points[i].point;
                }
                return nP;
            }
        }
        
        public Bounds3 Extents
        {
            get
            {
                if (extentsDirty)
                {
                    extentsDirty = false;
                    extents = new Bounds3(points[Length - 1].point, Float3.zero);
                    left = right = top = bottom = front = back =
                    smoothLeft = smoothRight = smoothTop = smoothBottom = smoothFront = smoothBack = Length - 1;

                    Float3[] sP = SmoothedPoints;

                    int currentSegment = newIDS[Length - 1];

                    Float3 min = extents.Min, max = extents.Max;

                    for (int i = Length - 2; i >= 0; i--)
                    {

                        if (points[i].point.x < points[left].point.x)
                        {
                            left = i;
                        }
                        else if (points[i].point.x == points[left].point.x &&
                                 points[i].point.y < points[left].point.y)
                        {
                            left = i;
                        }

                        if (points[i].point.y < points[bottom].point.y)
                        {
                            bottom = i;
                        }
                        else if (points[i].point.y == points[bottom].point.y &&
                                 points[i].point.x > points[bottom].point.x)
                        {
                            bottom = i;
                        }

                        if (points[i].point.z < points[back].point.z)
                        {
                            back = i;
                        }
                        else if (points[i].point.z == points[back].point.z &&
                                 points[i].point.y > points[back].point.y)
                        {
                            back = i;
                        }

                        if (points[i].point.x > points[right].point.x)
                        {
                            right = i;
                        }
                        else if (points[i].point.x == points[right].point.x &&
                                 points[i].point.y > points[right].point.y)
                        {
                            right = i;
                        }

                        if (points[i].point.y > points[top].point.y)
                        {
                            top = i;
                        }
                        else if (points[i].point.y == points[top].point.y &&
                                 points[i].point.x < points[top].point.x)
                        {
                            top = i;
                        }

                        if (points[i].point.z > points[front].point.z)
                        {
                            front = i;
                        }
                        else if (points[i].point.z == points[front].point.z &&
                                 points[i].point.y > points[front].point.y)
                        {
                            front = i;
                        }

                        int nextSegment = newIDS[i];

                        for (int j = currentSegment - 1; j >= nextSegment; j--)
                        {

                            if (sP[j].x < min.x || (sP[j].x == min.x && sP[j].y < min.y))
                            {
                                min.x = sP[j].x;
                                smoothLeft = j;
                            }


                            if (sP[j].y < min.y || (sP[j].y == min.y && sP[j].x > max.x))
                            {
                                min.y = sP[j].y;
                                smoothBottom = j;
                            }

                            if (sP[j].z < min.z || (sP[j].z == min.z && sP[j].y < min.y))
                            {
                                min.z = sP[j].z;
                                smoothBack = j;
                            }

                            if (sP[j].x > max.x || (sP[j].x == max.x && sP[j].y > max.y))
                            {
                                max.x = sP[j].x;
                                smoothRight = j;
                            }

                            if (sP[j].y > max.y || (sP[j].y == max.y && sP[j].x < min.x))
                            {
                                max.y = sP[j].y;
                                smoothTop = j;
                            }

                            if (sP[j].z > max.z || (sP[j].z == max.z && sP[j].y > max.y))
                            {
                                max.z = sP[j].z;
                                smoothFront = j;
                            }
                        }
                        currentSegment = nextSegment;
                    }
                    extents.SetMinMax(min, max);
                }
                return extents;
            }
        }

        public Float3 Min { get { return min; } }

        public Float3 Max { get { return max; } }

        public Float3 SmoothMin { get { return smoothMin; } }

        public Float3 SmoothMax { get { return smoothMax; } }

        public Float3[] SmoothedPoints
        {
            get
            {
                if (anchorsDirty)
                {
                    SetCapPoints();
                    smoothedPoints = GetSmoothedPoints(out newIDS);
                    anchorsDirty = false;
                }
                return smoothedPoints;
            }
        }

        public override int Length
        {
            get { if (wrap) { return points.Length - 1; } else { return points.Length; } }
        }

        public override bool Wrap
        {
            get { return wrap; }
            set
            {
                if (value != wrap)
                {
                    if (value)
                    {
                        if ((!points[0].point.Equals(points[points.Length - 1].point)))
                        {
                            System.Array.Resize(ref points, points.Length + 1);
                            points[points.Length - 1] = points[0];
                        }
                    }
                    else
                    {
                        if ((points[0].point.Equals(points[points.Length - 1].point)))
                        {
                            System.Array.Resize(ref points, points.Length - 1);
                        }
                    }
                    Sections = points.Length - 1;
                    anchorsDirty = true;
                    extentsDirty = true;
                    SetCapPoints();
                    GetTotalWeight();
                    GetTotalSplineLength(1);
                }
                wrap = value;
            }
        }

        public override int Sections
        {
            get { return sections.Length; }
            set
            {
                if (value != sections.Length)
                {
                    int newLength = value;
                    if (newLength > points.Length - 1)
                    {
                        newLength = points.Length - 1;
                    }
                    if (newLength > 0)
                    {
                        SplineSection[] newSec = new SplineSection[newLength];

                        totalWeight = 0;

                        int lesser = sections.Length;
                        if (newLength < lesser)
                        {
                            lesser = newLength;
                        }
                        for (int i = lesser - 1; i >= 0; i--)
                        {
                            newSec[i] = sections[i];
                        }

                        if (newLength > lesser)
                        {
                            for (int i = lesser; i < newLength; i++)
                            {
                                newSec[i].curveType = defaultCurveType;
                                newSec[i].weight = defaultWeight;
                                totalWeight += defaultWeight;
                            }
                        }
                        sections = newSec;
                    }
                }
            }
        }

        public float TotalSplineLength
        {
            get { if (splineLength < 0) { splineLength = GetTotalSplineLength(0.1f); } return splineLength; }
        }
        #endregion

        #region Constructors
        public Spline3()
        {
            defaultCurveType = CurveType.Linear;
            defaultWeight = 1f;
            defaultDropValue = 0;

            points = new Point3[0];

            Sections = 0;

            splineLength = 0;

            anchorsDirty = extentsDirty = true;
        }

        public Spline3(int p, CurveType dst = CurveType.Linear)
        {
            if (p <= 1) { p = 2; }
            //Debug.Log ("Spline : " + p);

            points = new Point3[p];

            float inv = 1f / p;
            for (int i = p - 1; i >= 0; i--)
            {
                points[i].point = new Float3(inv * i, inv * i, inv * i);
            }

            defaultCurveType = dst;
            defaultWeight = 1f;
            defaultDropValue = 0;

            Sections = p - 1;
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = extentsDirty = true;
        }
        public Spline3(Float3[] p, CurveType curveType = CurveType.Linear, bool doWrap = false)
        {
            defaultCurveType = curveType;
            defaultWeight = 1f;
            defaultDropValue = 0;
            wrap = doWrap;
            if (wrap)
            {
                points = new Point3[p.Length + 1];
                for (int i = p.Length - 1; i >= 0; i--)
                {
                    points[i] = new Point3();
                    points[i].point = p[i];
                }
                points[p.Length] = points[0];
                Wrap = true;
                Sections = p.Length;
            }
            else
            {
                points = new Point3[p.Length];
                for(int i = p.Length-1; i >=0; i--)
                {
                    points[i] = new Point3();
                    points[i].point = p[i];
                }
                Sections = p.Length - 1;
            }
            

            splineLength = 0;

            anchorsDirty = extentsDirty = true;
        }


        public Spline3(Spline3 spline, int p = -1, bool weighted = true, bool guaranteeResample = false)
        {
            if (p < 0) { p = spline.Length; }
            if (p < 2) { p = 2; }
            spline.GetTotalWeight();
            spline.GetTotalSplineLength(0.01f);
            if (p == spline.Length && !guaranteeResample)
            {

                name = spline.name;
                spline.GetAnchorPoints(out points);
                sections = spline.sections;

                wrap = spline.wrap;
                left = spline.left;
                right = spline.right;
                top = spline.top;
                bottom = spline.bottom;

            }
            else
            {
                points = new Point3[p];

                wrap = spline.wrap;
                float per = 1f / p;
                int s = -1;
                if (p > 2)
                {

                    for (int i = 0; i < p - 1; i++)
                    {
                        s = spline.GetSectionAtPercent(per * i, weighted);

                        points[i].point = spline.GetPoint(per * i, weighted);
                        if (!weighted)
                        {
                            sections[i].weight = 1f;
                        }
                        else
                        {
                            sections[i].weight = per * spline.sections[s].weight;
                        }
                        totalWeight += sections[i].weight;
                        sections[i].curveType = spline.sections[s].curveType;
                    }
                    points[p - 1] = spline.points[spline.Length - 1];
                }
                else
                {
                    points[0] = spline.points[0];
                    sections[0].weight = spline.sections[0].weight;

                    sections[0].curveType = spline.sections[0].curveType;
                    points[1] = spline.points[spline.Length - 1];
                }
                if (wrap && (!points[0].point.Equals(points[points.Length - 1].point)))
                {
                    System.Array.Resize(ref points, points.Length + 1);
                    points[points.Length - 1] = points[0];

                }
            }
            splineLength = GetTotalSplineLength(0.01f);
            anchorsDirty = extentsDirty = true;
        }
        #endregion

        #region Spline Methods

        public void GetMinMax()
        {
            min = points[points.Length - 1].point;
            max = points[points.Length - 1].point;

            Float3[] sP = SmoothedPoints;

            smoothMin = sP[sP.Length - 1];
            smoothMax = sP[sP.Length - 1];

            for (int i = Length - 1; i >= 0; i--)
            {

                if (points[i].point.x < min.x)
                {
                    min.x = points[i].point.x;
                }
                else if (points[i].point.x > max.x)
                {
                    max.x = points[i].point.x;
                }

                if (points[i].point.y < min.y)
                {
                    min.y = points[i].point.y;
                }
                else if (points[i].point.y > max.y)
                {
                    max.y = points[i].point.y;
                }

                if (points[i].point.z < min.z)
                {
                    min.z = points[i].point.z;
                }
                else if (points[i].point.z > max.z)
                {
                    max.z = points[i].point.z;
                }
            }

            for (int i = sP.Length - 1; i >= 0; i--)
            {

                if (sP[i].x < smoothMin.x)
                {
                    smoothMin.x = sP[i].x;
                }
                else if (sP[i].x > smoothMax.x)
                {
                    smoothMax.x = sP[i].x;
                }

                if (sP[i].y < smoothMin.y)
                {
                    smoothMin.y = sP[i].y;
                }
                else if (sP[i].y > smoothMax.y)
                {
                    smoothMax.y = sP[i].y;
                }

                if (sP[i].z < smoothMin.z)
                {
                    smoothMin.z = sP[i].z;
                }
                else if (sP[i].z > smoothMax.z)
                {
                    smoothMax.z = sP[i].z;
                }
            }
        }

        public void SetAnchorPoints(Float3[] newPoints)
        {
            points = new Point3[newPoints.Length];
            for(int i =0; i < newPoints.Length; i++)
            {
                points[i] = new Point3() { point = newPoints[i] };
            }
            if (wrap && !points[0].point.Equals(points[points.Length - 1].point))
            {
                System.Array.Resize(ref points, points.Length + 1);
                points[points.Length - 1] = points[0];
            }


            Sections = points.Length - 1;
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = extentsDirty = true;
        }

        public void SetAnchorPoints(Point3[] newPoints)
        {
            points = newPoints;
            if (wrap && !points[0].point.Equals(points[points.Length - 1].point))
            {
                System.Array.Resize(ref points, points.Length + 1);
                points[points.Length - 1] = points[0];
            }


            Sections = points.Length - 1;
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = extentsDirty = true;
        }

        public void GetAnchorPoints(out Point3[] p)
        {
            int l = points.Length;
            p = new Point3[l];
            for (int i = points.Length - 1; i >= 0; i--)
            {
                p[i] = points[i];
            }
        }

        public override void Scale(float scalar)
        {
            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i].point *= scalar;
                if (points[i].useBackHandle) points[i].backHandle *= scalar;
                if (points[i].useForeHandle) points[i].foreHandle *= scalar;
            }
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = true;
        }

        public override void Normalize()
        {
            GetMinMax();
            Float3 offset = smoothMin; offset *= -1;
            Float3 scalar = smoothMax - smoothMin;
            if (scalar.x == 0) { scalar.x = 1; } else { scalar.x = 1f / scalar.x; }
            if (scalar.y == 0) { scalar.y = 1; } else { scalar.y = 1f / scalar.y; }
            if (scalar.z == 0) { scalar.z = 1; } else { scalar.z = 1f / scalar.z; }

            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i].point += offset;
                points[i].point *= scalar;
                if (points[i].useBackHandle) points[i].backHandle *= scalar;
                if (points[i].useForeHandle) points[i].foreHandle *= scalar;
            }
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = true;
        }

        public void SetXOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.x = vals[i];
            }
        }

        public void SetYOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.y = vals[i];
            }
        }

        public void SetZOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.z = vals[i];
            }
        }

        public void AddPoints(Float3[] p, CurveType[] curves, float[] weights, int id = -1)
        {
            //Revamp this to do all the adding work itself, instead of repeatedly calling AddPoint.
            int npc = points.Length;
            int fillerSlots = 0;
            if (id > npc) { fillerSlots = id - npc; }

            totalWeight = 0;
            List<Point3> newPoints = new List<Point3>(points);
            List<SplineSection> newSS = new List<SplineSection>(sections);

            for (int i = 0; i < p.Length; i++)
            {

                CurveType curve = curves[i];
                float weight = weights[i];
                if (curves[i] == CurveType.None)
                {
                    curve = defaultCurveType;
                }
                if (weights[i] < 0)
                {
                    weight = defaultWeight;
                }

                if (id < 0)
                {
                    Point3 p2 = new Point3(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.weight = weight;
                    ss.curveType = curve;
                    newPoints.Add(p2); newSS.Add(ss);
                    totalWeight += weight;

                }
                else if (id < points.Length)
                {
                    Point3 p2 = new Point3(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.curveType = curve;
                    newPoints.Insert(id, p2); newSS.Insert(id, ss);
                    totalWeight += weight;

                }
                else
                {
                    for (int j = 0; j < fillerSlots; j++)
                    {
                        SplineSection se = new SplineSection();
                        se.weight = defaultWeight;
                        se.curveType = defaultCurveType;
                        newPoints.Add(new Point3()); newSS.Add(se);
                        totalWeight += defaultWeight;

                    }
                    Point3 p2 = new Point3(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.weight = weight;
                    ss.curveType = curve;
                    newPoints.Add(p2); newSS.Add(ss);

                }
            }
            points = newPoints.ToArray();
            sections = newSS.ToArray();

            anchorsDirty = true;
            extentsDirty = true;
            SetCapPoints();
            GetTotalWeight();
            GetTotalSplineLength(1);
        }

        public void AddPoint(Float3 p, int id = -1, CurveType s = CurveType.None, float t = -1f)
        {
            if (s == CurveType.None)
            {
                s = defaultCurveType;
            }
            if (t < 0)
            {
                t = defaultWeight;
            }


            int npc = points.Length;
            int fillerSlots = 0;
            if (id > npc) { fillerSlots = id - npc; }

            totalWeight = 0;
            List<Point3> newPoints = new List<Point3>(points);
            List<SplineSection> newSS = new List<SplineSection>(sections);

            if (id < 0)
            {
                Point3 p2 = new Point3(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.curveType = s;
                ss.weight = t;
                newPoints.Add(p2); newSS.Add(ss);
                totalWeight += t;

            }
            else if (id < points.Length)
            {
                Point3 p2 = new Point3(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.weight = t;
                ss.curveType = s;
                newPoints.Insert(id, p2); newSS.Insert(id, ss);
                totalWeight += t;

            }
            else
            {
                for (int i = 0; i < fillerSlots; i++)
                {
                    SplineSection se = new SplineSection();
                    se.weight = defaultWeight;
                    se.curveType = defaultCurveType;
                    newPoints.Add(new Point3()); newSS.Add(se);
                    totalWeight += defaultWeight;

                }
                Point3 p2 = new Point3(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.weight = t;
                ss.curveType = s;
                newPoints.Add(p2); newSS.Add(ss);

            }


            points = newPoints.ToArray();

            sections = newSS.ToArray();

            anchorsDirty = true;
            extentsDirty = true;
            SetCapPoints();
            GetTotalWeight();
            GetTotalSplineLength(1);
        }

        public override void RemovePoint(int id)
        {

            if (id >= 0 && id < points.Length)
            {
                List<Point3> newPoints = new List<Point3>(points);

                List<SplineSection> newSections = new List<SplineSection>(sections);

                newPoints.RemoveAt(id);
                if (id < points.Length - 1)
                {
                    newSections.RemoveAt(id);
                }
                points = newPoints.ToArray();

                sections = newSections.ToArray();

                totalWeight = 0;
                for (int i = sections.Length - 1; i >= 0; i--)
                {
                    totalWeight += sections[i].weight;
                }

            }
        }

        public override void RemovePoints(int[] ids)
        {
            List<Point3> newPoints = new List<Point3>(points);
            List<SplineSection> newSections = new List<SplineSection>(sections);

            for (int i = Length - 1; i >= 0; i--)
            {

                for (int j = 0; j < ids.Length; j++)
                {
                    if (i == ids[j])
                    {
                        newPoints.RemoveAt(ids[j]);
                        if (i < points.Length - 1) newSections.RemoveAt(ids[j]);

                    }
                }

            }

            points = newPoints.ToArray();

            sections = newSections.ToArray();

            totalWeight = 0;
            for (int i = sections.Length - 1; i >= 0; i--)
            {
                totalWeight += sections[i].weight;
            }
        }

        public override void SetCapPoints()
        {
            //First, how many points do we actually have?
            int l = points.Length;

            //We need at least two points.
            if (l > 1)
            {
                //Our starting control point is the difference between point 0 and point 1 subtracted from point 0.
                Float3 pointDiff = points[1].point; pointDiff -= points[0].point;
                startCap = points[0].point; startCap -= pointDiff;

                if (l >= 3)
                {
                    //We check to see if this is a loop - if the first and last official points are the same.
                    // If so, we set our control points to form that perfect, continuous loop.
                    if (points[0].point.Equals(points[l - 1].point))
                    {
                        startCap = points[l - 2].point;
                        endCap = points[1].point;
                    }
                    //If not, we create our two control points. Each is just a straight line from our second set of points, going inward, through our first 
                    //set of points.	
                    else
                    {
                        pointDiff = points[l - 1].point - points[l - 2].point;
                        endCap = points[l - 1].point; endCap += pointDiff;
                    }
                }
                else
                {
                    endCap = points[1].point; endCap += pointDiff;
                }
            }
        }

        public Float3 GetPoint(float percent, bool weighted = true)
        {

            Float3 currentPoint;
            if (points.Length > 0)
            {
                if (points.Length > 1)
                {
                    if (percent <= 0)
                    {
                        currentPoint = points[0].point;
                    }
                    else if (percent >= 1)
                    {
                        currentPoint = points[points.Length - 1].point;
                    }
                    else
                    {
                        int cS = GetSectionAtPercent(percent, weighted);

                        if (cS != currentSection)
                        {
                            currentSection = cS;
                            GetControlPoints();
                        }
                        Float3 point = Float3.zero;
                        switch (sections[cS].curveType)
                        {
                            //Catmull Rom
                            case CurveType.CatmullRom:
                                point.x = Splines.CatmullRom(weightedPercent, a.x, b.x, c.x, d.x);
                                point.y = Splines.CatmullRom(weightedPercent, a.y, b.y, c.y, d.y);
                                point.z = Splines.CatmullRom(weightedPercent, a.z, b.z, c.z, d.z);
                                break;

                            //Linear
                            case CurveType.Linear:
                                point = Float3.Lerp(points[cS].point, points[cS + 1].point, weightedPercent);

                                break;

                            //CubicBezier
                            case CurveType.CubicBezier:
                                point.x = Splines.CubicBezier(weightedPercent, a.x, b.x, c.x, d.x);
                                point.y = Splines.CubicBezier(weightedPercent, a.y, b.y, c.y, d.y);
                                point.z = Splines.CubicBezier(weightedPercent, a.z, b.z, c.z, d.z);
                                break;

                            //Step
                            case CurveType.Step:
                                point = points[cS].point;
                                break;
                        }
                        currentPoint = point;
                    }

                }
                else
                {
                    currentPoint = points[0].point;
                }
            }
            else
            {
                currentPoint = Float3.zero;
            }
            return currentPoint;
        }

        public override void GetControlPoints()
        {
            int l = sections.Length;
            if (currentSection == 0 && l > 1)
            {
                a = startCap;
                b = points[0].point;
                c = points[1].point;
                d = points[2].point;
            }
            else if (currentSection == 0)
            {
                a = startCap;
                b = points[0].point;
                c = points[1].point;
                d = endCap;
            }
            else if (currentSection == l - 1)
            {
                a = points[l - 2].point;
                b = points[l - 1].point;
                c = points[l].point;
                d = endCap;
            }
            else if (currentSection > 0)
            {
                a = points[currentSection - 1].point;
                b = points[currentSection].point;
                c = points[currentSection + 1].point;
                d = points[currentSection + 2].point;
            }

            if (points[currentSection].useBackHandle)
            {
                a = points[currentSection].point + points[currentSection].backHandle;
            }
            if (points[currentSection + 1].useForeHandle)
            {
                d = points[currentSection + 1].point + points[currentSection + 1].foreHandle;
            }
        }

        public Spline3 SplitSpline(int id, bool includeCurrent)
        {

            if (points.Length > id)
            {
                List<Point3> newPoints = new List<Point3>(points);

                List<SplineSection> newSections = new List<SplineSection>(sections);

                int diff = Length - id;
                List<Point3> splitPoints = newPoints.GetRange(id, diff);
                List<SplineSection> splitSections = newSections.GetRange(id, diff);


                points = newPoints.GetRange(0, id - 1).ToArray();
                sections = newSections.GetRange(0, id - 1).ToArray();

                totalWeight = 0;
                for (int i = sections.Length - 1; i >= 0; i--)
                {
                    totalWeight += sections[i].weight;
                }

                Spline3 leftOvers = new Spline3();
                leftOvers.SetAnchorPoints(splitPoints.ToArray());
                leftOvers.sections = splitSections.ToArray();

                leftOvers.totalWeight = 0;
                for (int i = leftOvers.sections.Length - 1; i >= 0; i--)
                {
                    leftOvers.totalWeight += leftOvers.sections[i].weight;
                }
                return leftOvers;
            }
            else
            {
                return new Spline3();
            }
        }

        public Spline3 GetSubSpline(int start, int end)
        {
            List<Point3> newPoints = new List<Point3>(points);

            List<SplineSection> newSections = new List<SplineSection>(sections);

            int diff = end - start;
            int ddif = diff - 1;
            Spline3 s = new Spline3();
            if (start > end)
            {
                if (!wrap) { return null; }
                //diff = points.Length - 1 - start;
                diff = points.Length - start;
                ddif = diff - 1;
                newPoints = newPoints.GetRange(start, diff);
                newSections = newSections.GetRange(start, ddif);

                for (int i = 1; i <= end; i++)
                {
                    newPoints.Add(points[i]);

                    newSections.Add(sections[i]);

                }
            }
            else
            {
                newPoints = newPoints.GetRange(start, diff);
                newSections = newSections.GetRange(start, ddif);
            }

            s.points = newPoints.ToArray();
            s.sections = newSections.ToArray();

            if (start > 0)
            {
                s.startCap = points[start - 1].point;

            }
            else { s.startCap = startCap; }
            if (end < points.Length - 1)
            {
                s.endCap = points[end + 1].point;

            }
            else { s.endCap = new Float3(endCap); }

            s.anchorsDirty = true;

            s.Sections = newSections.Count;
            s.GetTotalWeight();
            s.GetTotalSplineLength(0.01f, true);
            return s;
        }

        //I should get either ClosestPolyPoint or this working for both circumstances. Start by checking to see which works better.
        public int GetClosestAnchorPoint(Float3 p)
        {
            float lowestDist = float.PositiveInfinity;
            int id = -1;
            for (int i = Length - 1; i >= 0; i--)
            {

                float d = Float3.Distance(p, this[i]);
                if (d < lowestDist)
                {
                    id = i;
                    lowestDist = d;
                }
            }
            return id;
        }

        public Float3 ClosestPointOnSpline(Float3 point, int samples, ref int closestAnchor, bool weighted = true)
        {
            closestAnchor = points.Length - 2;
            int nextClosestAnchor = -1;

            float currentDistance = Float3.Distance(point, points[points.Length - 1].point),
                currentPer = 1f, per, nextPer;

            Float3 p1, p2 = points[points.Length - 2].point, p3 = points[points.Length - 1].point;

            nextPer = 1;

            for (int i = points.Length - 2; i >= 0; i--)
            {
                per = GetPercentAtPoint(i, weighted);
                float incr = (nextPer - per) / samples;

                for (int j = samples - 1; j >= 0; j--)
                {
                    Float3 tp2 = GetPoint(per + (incr * j));
                    float dist = Float3.Distance(point, tp2);
                    if (dist < currentDistance)
                    {
                        p2 = tp2;
                        currentDistance = dist;
                        currentPer = per + (incr * j);
                        nextClosestAnchor = closestAnchor;
                        closestAnchor = i;
                    }
                }
                nextPer = per;
            }

            p1 = points[closestAnchor].point;
            if (nextClosestAnchor > closestAnchor)
            {
                p3 = points[closestAnchor + 1].point;
            }
            else
            {
                p3 = points[closestAnchor - 1].point;
            }

            per = GetPercentAtPoint(closestAnchor, weighted);
            currentPer = (nextPer + per) * 0.5f;
            p2 = GetPoint(currentPer, weighted);

            for (int i = samples; i > 0; i--)
            {
                float dist1 = Float3.Distance(p1, point),
                      dist2 = Float3.Distance(p3, point);
                if (dist1 < dist2)
                {
                    p3 = p2;
                    nextPer = currentPer;
                    currentPer = (per + currentPer) * 0.5f;
                }
                else
                {
                    p1 = p2;
                    per = currentPer;
                }
                currentPer = (nextPer + per) * 0.5f;
                p2 = GetPoint(currentPer, weighted);
            }
            return p2;
        }

        public override float GetTotalSplineLength(float percent, bool resetHeldValue = false)
        {

            float sLength = 0f;

            if (sections.Length > 0)
            {

                float secPer = 0;
                float sp = 0;
                for (int j = 0; j < sections.Length; j++)
                {
                    sections[j].length = 0;
                    if (sections[j].curveType == CurveType.Linear)
                    {
                        sections[j].length = Float3.Distance(points[j].point, points[j + 1].point);
                        sLength += sections[j].length;
                    }
                    else if (sections[j].curveType == CurveType.CatmullRom || sections[j].curveType == CurveType.CubicBezier)
                    {

                        Float3 a = points[j].point, b;
                        secPer += sections[j].weight;
                        float isp = secPer / totalWeight;

                        for (float i = percent; i <= 1; i += percent)
                        {
                            b = GetPoint(Float.Lerp(sp, isp, i));
                            float d = Float3.Distance(a, b);
                            a = b;
                            sections[j].length += d;
                            sLength += d;
                        }
                        sp = isp;
                    }
                }
            }
            if (resetHeldValue) { splineLength = sLength; }
            return sLength;
        }

        public Float3 AveragePoints(bool useSmoothed = true)
        {

            Float3 average = Float3.zero;

            if (useSmoothed)
            {
                Float3[] pts = SmoothedPoints;
                for (int i = 0; i < pts.Length; i++)
                {
                    average += pts[i];
                }
                average /= pts.Length;
            }
            else
            {
                for (int i = 0; i < points.Length; i++)
                {
                    average += points[i].point;
                }
                average /= points.Length;
            }

            return average;
        }

        public void Offset(Float3 offset)
        {
            for (int i = Length - 1; i >= 0; i--)
            {
                this[i] += offset;
            }
            anchorsDirty = true;
            extentsDirty = true;
        }

        public override void ResamplePoints(int samples)
        {
            if (samples > 2)
            {

                float perc = 1f / (samples - 1);

                Point3[] newPoints = new Point3[samples];
                SplineSection[] secs = new SplineSection[samples - 1];

                newPoints[0] = points[0];
                secs[0] = sections[0];

                int s = 0;
                for (int i = 1; i < points.Length; i++)
                {
                    float ip = GetPercentAtPoint(i), sp = s * perc;
                    while (sp < ip)
                    {
                        s++;
                        newPoints[s] = points[i];
                        newPoints[s].point = GetPoint(sp);
                        if (i < points.Length - 1) { secs[s] = sections[i]; }
                        sp = s * perc;
                    }
                    if (s == samples) { break; }
                }

                points = newPoints;
                sections = secs;
            }
        }

        public override float Distance(float p1, float p2)
        {
            return Distance(p1, p2, 20);
        }

        public override float Distance(float p1, float p2, int samples)
        {
            float distance = 0f;
            float p = p2 - p1 / samples;
            float cP = p1;
            Float3 currentPoint = GetPoint(p1);
            Float3 prevPoint = currentPoint;

            for (int i = samples - 1; i >= 0; i--)
            {
                cP += p;
                currentPoint = GetPoint(cP);
                distance += Float3.Distance(prevPoint, currentPoint);
                prevPoint = currentPoint;
            }
            return distance;
        }

        public Float3[] GetSmoothedPoints(out int[] newIDS, int maxSubdivisions = 10, float subDivideAngle = 15)
        {
            if (maxSubdivisions < 3) { maxSubdivisions = 3; }
            
            List<Float3> newPoints = new List<Float3>();
            List<int> nIDS = new List<int>();

            for (int i = 0; i < points.Length - 1; i++)
            {
                nIDS.Add(newPoints.Count);
                newPoints.Add(points[i].point);

                switch (sections[i].curveType)
                {
                    case CurveType.CatmullRom:
                    case CurveType.CubicBezier:

                        currentSection = -1;
                        float pa = GetPercentAtPoint(i), pb = GetPercentAtPoint(i + 1);
                        
                        float diff = pb - pa;
                        float per = diff / maxSubdivisions;

                        for (int j = 1; j < maxSubdivisions; j++)
                        {
                            newPoints.Add(GetPoint(pa + (per * j)));
                        }

                        break;
                }
            }
            nIDS.Add(newPoints.Count);
            newPoints.Add(points[points.Length - 1].point);

            newIDS = nIDS.ToArray();
            return newPoints.ToArray();
        }

        public override int GetSectionID(int smoothID)
        {

            int currentSec = newIDS[points.Length - 1];
            if (smoothID <= currentSec)
            {
                for (int i = points.Length - 2; i >= 0; i--)
                {
                    if (smoothID < currentSec)
                    {
                        return i;
                    }
                    currentSec = newIDS[i];
                }
                return points.Length - 1;
            }
            return -1;
        }

        public void RotateSpline(Float3 angle)
        {
            Float3 c = extents.Center;
            for (int i = points.Length - 1; i >= 0; i--)
            {
                Float3 p = points[i].point;
                points[i].point = p.RotateAround(c, angle);
            }
        }

        public void ScaleSpline(Float3 scale)
        {
            Float3 c = extents.Center;
            for (int i = points.Length - 1; i >= 0; i--)
            {
                Float3 p = points[i].point;
                p -= c;
                p *= scale;
                p += c;
                points[i].point = p;
            }
        }

        public Float3 GetClosestPointOnSpline(Float3 point, int samples, ref int closestSectionBeforePoint)
        {
            int closestSection = GetClosestAnchorPoint(point);

            int aSection = closestSection - 1, bSection = closestSection +1;

            Float3 aPoint = points[aSection].point, 
                   bPoint = points[bSection].point;
            
            float aDist = Float3.Distance(point, points[aSection].point),
                  bDist = Float3.Distance(point, points[bSection].point);

            if(aDist < bDist)
            {
                closestSectionBeforePoint = aSection;
                bSection = closestSection;
                bPoint = points[bSection].point;
                bDist = Float3.Distance(point, bPoint);
            }
            else
            {
                closestSectionBeforePoint = closestSection;
                aSection = closestSection;
                aPoint = points[aSection].point;
                aDist = Float3.Distance(point, aPoint);
            }

            //Sample halfway between A and B. Find which two are closest, then resample and continue.
            //Return the point we arrive at when we're out of samples.
            
            float aPercent = GetPercentAtPoint(aSection), bPercent = GetPercentAtPoint(bSection);
            float halfPercent = (aPercent + bPercent) * 0.5f;

            Float3 halfway = GetPoint(halfPercent);
            float halfDist = Float3.Distance(point, halfway);

            for(int i =0; i < samples; i++)
            {
                if(aDist < bDist)
                {
                    //Halfway becomes B. Resample halfway.
                    bPoint = halfway;
                    bPercent = halfPercent;
                    bDist = halfDist;

                    halfPercent = (aPercent + bPercent) * 0.5f;
                    halfway = GetPoint(halfPercent);
                    halfDist = Float3.Distance(point, halfway);
                }
                else
                {
                    //Halfway becomes A. Resample Halfway.
                    aPoint = halfway;
                    aPercent = halfPercent;
                    aDist = halfDist;

                    halfPercent = (aPercent + bPercent) * 0.5f;
                    halfway = GetPoint(halfPercent);
                    halfDist = Float3.Distance(point, halfway);
                }
            }

            if(aDist < bDist)
            {
                if(aDist < halfDist)
                {
                    return aPoint;
                }
                
            }
            else
            {
                if(bDist < halfDist)
                {
                    return bPoint;
                }
            }
            return halfway;
        }
        #endregion

    }

    [System.Serializable]
    public class Spline4 : SplineBase
    {
        #region Variables

        public Point4[] points = new Point4[0];

        public Bounds4 extents;
        public bool extentsDirty = true;

        public int top, smoothTop, left, smoothLeft, bottom, smoothBottom, right, smoothRight,
            front, smoothFront, back, smoothBack, ahead, smoothAhead, behind, smoothBehind;

        public Float4[] smoothedPoints = new Float4[0];

        public Float4 startCap, endCap;

        Float4 a, b, c, d;

        Float4 min, max, smoothMin, smoothMax;

        float splineLength = -1;

        #endregion

        #region Properties
        /// <summary>
        /// The top-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Top { get { if (extentsDirty) { Bounds4 e = Extents; } return top; } }

        public int SmoothTop { get { if (extentsDirty) { Bounds4 e = Extents; } return smoothTop; } }

        /// <summary>
        /// The left-most point. If two or more points share the same min X value, the bottom-most point is used.
        /// </summary>
        public int Left { get { if (extentsDirty) { Bounds4 e = Extents; } return left; } }

        public int SmoothLeft { get { if (extentsDirty) { Bounds4 e = Extents; } return smoothLeft; } }

        /// <summary>
        /// The bottom-most point. If two or more points share the same min Y value, the right-most point is used.
        /// </summary>
        public int Bottom { get { if (extentsDirty) { Bounds4 e = Extents; } return bottom; } }

        public int SmoothBottom { get { if (extentsDirty) { Bounds4 e = Extents; } return smoothBottom; } }

        /// <summary>
        /// The right-most point. If two or more points share the same max X value, the top-most point is used.
        /// </summary>
        public int Right { get { if (extentsDirty) { Bounds4 e = Extents; } return right; } }

        public int SmoothRight { get { if (extentsDirty) { Bounds4 e = Extents; } return smoothRight; } }

        /// <summary>
        /// The front-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Front { get { if (extentsDirty) { Bounds4 e = Extents; } return front; } }

        public int SmoothFront { get { if (extentsDirty) { Bounds4 e = Extents; } return smoothFront; } }

        /// <summary>
        /// The back-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Back { get { if (extentsDirty) { Bounds4 e = Extents; } return back; } }

        public int SmoothBack { get { if (extentsDirty) { Bounds4 e = Extents; } return smoothBack; } }

        /// <summary>
        /// The ahead-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Ahead { get { if (extentsDirty) { Bounds4 e = Extents; } return ahead; } }

        public int SmoothAhead { get { if (extentsDirty) { Bounds4 e = Extents; } return smoothAhead; } }

        /// <summary>
        /// The behind-most point. If two or more points share the same max Y value, the left-most point is used.
        /// </summary>
        public int Behind { get { if (extentsDirty) { Bounds4 e = Extents; } return behind; } }

        public int SmoothBehind { get { if (extentsDirty) { Bounds4 e = Extents; } return smoothBehind; } }

        public Float4 this[int index]
        {
            get { return points[index].point; }
            set
            {
                anchorsDirty = true;
                if (wrap && (index == 0 || index == Length))
                {
                    points[0].point = points[Length].point = value;
                }
                else
                {
                    points[index].point = value;

                }
            }
        }

        public Float4[] AnchorPoints
        {
            get
            {
                Float4[] nP = new Float4[points.Length];
                for (int i = nP.Length - 1; i >= 0; i--)
                {
                    nP[i] = points[i].point;
                }
                return nP;
            }
        }

        public Bounds4 Extents
        {
            get
            {
                if (extentsDirty)
                {
                    extentsDirty = false;
                    extents = new Bounds4(points[Length - 1].point, Float4.zero);
                    left = right = top = bottom = front = back = ahead = behind =
                    smoothLeft = smoothRight = smoothTop = smoothBottom = smoothFront =
                    smoothBack = smoothAhead = smoothBehind = Length - 1;

                    Float4[] sP = SmoothedPoints;

                    int currentSegment = newIDS[Length - 1];

                    Float4 min = extents.Min, max = extents.Max;

                    for (int i = Length - 2; i >= 0; i--)
                    {

                        if (points[i].point.x < points[left].point.x)
                        {
                            left = i;
                        }
                        else if (points[i].point.x == points[left].point.x &&
                                 points[i].point.y < points[left].point.y)
                        {
                            left = i;
                        }

                        if (points[i].point.y < points[bottom].point.y)
                        {
                            bottom = i;
                        }
                        else if (points[i].point.y == points[bottom].point.y &&
                                 points[i].point.x > points[bottom].point.x)
                        {
                            bottom = i;
                        }

                        if (points[i].point.z < points[back].point.z)
                        {
                            back = i;
                        }
                        else if (points[i].point.z == points[back].point.z &&
                                 points[i].point.w > points[back].point.w)
                        {
                            back = i;
                        }

                        if (points[i].point.w < points[behind].point.w)
                        {
                            behind = i;
                        }
                        else if (points[i].point.w == points[behind].point.w &&
                                 points[i].point.z > points[behind].point.z)
                        {
                            behind = i;
                        }

                        if (points[i].point.x > points[right].point.x)
                        {
                            right = i;
                        }
                        else if (points[i].point.x == points[right].point.x &&
                                 points[i].point.y > points[right].point.y)
                        {
                            right = i;
                        }

                        if (points[i].point.y > points[top].point.y)
                        {
                            top = i;
                        }
                        else if (points[i].point.y == points[top].point.y &&
                                 points[i].point.x < points[top].point.x)
                        {
                            top = i;
                        }

                        if (points[i].point.z > points[front].point.z)
                        {
                            front = i;
                        }
                        else if (points[i].point.z == points[front].point.z &&
                                 points[i].point.y > points[front].point.y)
                        {
                            front = i;
                        }

                        if (points[i].point.w > points[ahead].point.w)
                        {
                            ahead = i;
                        }
                        else if (points[i].point.w == points[ahead].point.w &&
                                 points[i].point.z > points[ahead].point.z)
                        {
                            ahead = i;
                        }

                        int nextSegment = newIDS[i];

                        for (int j = currentSegment - 1; j >= nextSegment; j--)
                        {

                            if (sP[j].x < min.x || (sP[j].x == min.x && sP[j].y < min.y))
                            {
                                min.x = sP[j].x;
                                smoothLeft = j;
                            }


                            if (sP[j].y < min.y || (sP[j].y == min.y && sP[j].x > max.x))
                            {
                                min.y = sP[j].y;
                                smoothBottom = j;
                            }

                            if (sP[j].z < min.z || (sP[j].z == min.z && sP[j].w < min.w))
                            {
                                min.z = sP[j].z;
                                smoothBack = j;
                            }

                            if (sP[j].w < min.w || (sP[j].w == min.w && sP[j].y < min.y))
                            {
                                min.w = sP[j].w;
                                smoothBehind = j;
                            }

                            if (sP[j].x > max.x || (sP[j].x == max.x && sP[j].y > max.y))
                            {
                                max.x = sP[j].x;
                                smoothRight = j;
                            }

                            if (sP[j].y > max.y || (sP[j].y == max.y && sP[j].x < min.x))
                            {
                                max.y = sP[j].y;
                                smoothTop = j;
                            }

                            if (sP[j].z > max.z || (sP[j].z == max.z && sP[j].y > max.y))
                            {
                                max.z = sP[j].z;
                                smoothFront = j;
                            }

                            if (sP[j].w > max.w || (sP[j].w == max.w && sP[j].y > max.y))
                            {
                                max.w = sP[j].w;
                                smoothAhead = j;
                            }
                        }
                        currentSegment = nextSegment;
                    }
                    extents.SetMinMax(min, max);
                }
                return extents;
            }
        }

        public Float4 Min { get { return min; } }

        public Float4 Max { get { return max; } }

        public Float4 SmoothMin { get { return smoothMin; } }

        public Float4 SmoothMax { get { return smoothMax; } }

        public Float4[] SmoothedPoints
        {
            get
            {
                if (anchorsDirty)
                {
                    SetCapPoints();
                    smoothedPoints = GetSmoothedPoints(out newIDS);
                    anchorsDirty = false;
                }
                return smoothedPoints;
            }
        }

        public override int Length
        {
            get { if (wrap) { return points.Length - 1; } else { return points.Length; } }
        }

        public override bool Wrap
        {
            get { return wrap; }
            set
            {
                if (value != wrap)
                {
                    if (value)
                    {
                        if ((!points[0].point.Equals(points[points.Length - 1].point)))
                        {
                            System.Array.Resize(ref points, points.Length + 1);
                            points[points.Length - 1] = points[0];

                        }
                    }
                    else
                    {
                        if (points[0].point.Equals(points[points.Length - 1].point))
                        {
                            System.Array.Resize(ref points, points.Length - 1);
                        }
                    }
                    Sections = points.Length - 1;
                    anchorsDirty = true;
                    extentsDirty = true;
                    SetCapPoints();
                    GetTotalWeight();
                    GetTotalSplineLength(1);
                }
                wrap = value;
            }
        }

        public override int Sections
        {
            get { return sections.Length; }
            set
            {
                if (value != sections.Length)
                {
                    int newLength = value;
                    if (newLength > points.Length - 1)
                    {
                        newLength = points.Length - 1;
                    }

                    if (newLength > 0)
                    {
                        SplineSection[] newSec = new SplineSection[newLength];

                        totalWeight = 0;

                        int lesser = sections.Length;
                        if (newLength < lesser)
                        {
                            lesser = newLength;
                        }
                        for (int i = lesser - 1; i >= 0; i--)
                        {
                            newSec[i] = sections[i];
                        }

                        if (newLength > lesser)
                        {
                            for (int i = lesser; i < newLength; i++)
                            {
                                newSec[i].curveType = defaultCurveType;
                                newSec[i].weight = defaultWeight;
                                totalWeight += defaultWeight;
                            }
                        }
                        sections = newSec;
                    }
                }
            }
        }

        public float TotalSplineLength
        {
            get { if (splineLength < 0) { splineLength = GetTotalSplineLength(0.1f); } return splineLength; }
        }
        #endregion

        #region Constructors
        public Spline4()
        {
            defaultCurveType = CurveType.Linear;
            defaultWeight = 1f;
            defaultDropValue = 0;

            points = new Point4[0];

            Sections = 0;

            splineLength = 0;
            anchorsDirty = extentsDirty = true;
        }

        public Spline4(int p, CurveType dst = CurveType.Linear)
        {
            if (p <= 1) { p = 2; }
            //Debug.Log ("Spline : " + p);

            points = new Point4[p];

            float inv = 1f / p;
            for (int i = p - 1; i >= 0; i--)
            {
                points[i].point = new Float4(inv * i, inv * i, inv * i, inv * i);
            }

            defaultCurveType = dst;
            defaultWeight = 1f;
            defaultDropValue = 0;

            Sections = p - 1;
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = extentsDirty = true;
        }
        public Spline4(Float4[] p)
        {
            defaultCurveType = CurveType.CatmullRom;
            defaultWeight = 1f;
            defaultDropValue = -99;

            points = new Point4[p.Length];
            for (int i = p.Length - 1; i >= 0; i--)
            {
                points[i] = new Point4();
                points[i].point = p[i];
            }

            Wrap = false;

            Sections = p.Length - 1;

            SetCapPoints();
            GetTotalWeight();
            GetTotalSplineLength(1);

            splineLength = 0;
            anchorsDirty = extentsDirty = true;
        }

        public Spline4(Spline4 spline, int p = -1, bool weighted = true, bool guaranteeResample = false)
        {
            if (p < 0) { p = spline.Length; }
            if (p < 2) { p = 2; }
            spline.GetTotalWeight();
            spline.GetTotalSplineLength(0.01f);
            if (p == spline.Length && !guaranteeResample)
            {

                name = spline.name;
                spline.GetAnchorPoints(out points);
                sections = spline.sections;

                wrap = spline.wrap;
                left = spline.left;
                right = spline.right;
                top = spline.top;
                bottom = spline.bottom;
            }
            else
            {
                points = new Point4[p];

                wrap = spline.wrap;
                float per = 1f / p;
                int s = -1;
                if (p > 2)
                {

                    for (int i = 0; i < p - 1; i++)
                    {
                        s = spline.GetSectionAtPercent(per * i, weighted);

                        points[i].point = spline.GetPoint(per * i, weighted);
                        if (!weighted)
                        {
                            sections[i].weight = 1f;
                        }
                        else
                        {
                            sections[i].weight = per * spline.sections[s].weight;
                        }
                        totalWeight += sections[i].weight;
                        sections[i].curveType = spline.sections[s].curveType;
                    }
                    points[p - 1] = spline.points[spline.Length - 1];
                }
                else
                {
                    points[0] = spline.points[0];
                    sections[0].weight = spline.sections[0].weight;

                    sections[0].curveType = spline.sections[0].curveType;
                    points[1] = spline.points[spline.Length - 1];
                }
                if (wrap && !points[0].point.Equals(points[points.Length - 1].point))
                {
                    System.Array.Resize(ref points, points.Length + 1);
                    points[points.Length - 1] = points[0];

                }
            }
            splineLength = GetTotalSplineLength(0.01f);
            anchorsDirty = extentsDirty = true;
        }

        public static Spline4 Gradient(Float4[] colors)
        {
            if (colors.Length < 2)
            {
                colors = new Float4[]
                {
                    new Float4(1,0,0,1), new Float4(0,1,0,1)
                };
            }

            Float4[] hsvas = new Float4[colors.Length];
            for (int i = 0; i < colors.Length; i++)
            {
                hsvas[i] = Float4.RGBToHSV(colors[i]);
            }
            return new Spline4(hsvas);
        }
        #endregion

        #region Spline Methods
        public void GetMinMax()
        {
            min = points[points.Length - 1].point;
            max = points[points.Length - 1].point;

            Float4[] sP = SmoothedPoints;

            smoothMin = sP[sP.Length - 1];
            smoothMax = sP[sP.Length - 1];

            for (int i = Length - 1; i >= 0; i--)
            {

                if (points[i].point.x < min.x)
                {
                    min.x = points[i].point.x;
                }
                else if (points[i].point.x > max.x)
                {
                    max.x = points[i].point.x;
                }

                if (points[i].point.y < min.y)
                {
                    min.y = points[i].point.y;
                }
                else if (points[i].point.y > max.y)
                {
                    max.y = points[i].point.y;
                }

                if (points[i].point.z < min.z)
                {
                    min.z = points[i].point.z;
                }
                else if (points[i].point.z > max.z)
                {
                    max.z = points[i].point.z;
                }

                if (points[i].point.w < min.w)
                {
                    min.w = points[i].point.w;
                }
                else if (points[i].point.w > max.w)
                {
                    max.w = points[i].point.w;
                }
            }

            for (int i = sP.Length - 1; i >= 0; i--)
            {

                if (sP[i].x < smoothMin.x)
                {
                    smoothMin.x = sP[i].x;
                }
                else if (sP[i].x > smoothMax.x)
                {
                    smoothMax.x = sP[i].x;
                }

                if (sP[i].y < smoothMin.y)
                {
                    smoothMin.y = sP[i].y;
                }
                else if (sP[i].y > smoothMax.y)
                {
                    smoothMax.y = sP[i].y;
                }

                if (sP[i].z < smoothMin.z)
                {
                    smoothMin.z = sP[i].z;
                }
                else if (sP[i].z > smoothMax.z)
                {
                    smoothMax.z = sP[i].z;
                }

                if (sP[i].w < smoothMin.w)
                {
                    smoothMin.w = sP[i].w;
                }
                else if (sP[i].w > smoothMax.w)
                {
                    smoothMax.w = sP[i].w;
                }
            }
        }

        public void SetAnchorPoints(Point4[] newPoints)
        {
            points = newPoints;
            if (wrap && !points[0].point.Equals(points[points.Length - 1].point))
            {
                System.Array.Resize(ref points, points.Length + 1);
                points[points.Length - 1] = points[0];
            }


            Sections = points.Length - 1;
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = extentsDirty = true;
        }

        public void GetAnchorPoints(out Point4[] p)
        {
            int l = points.Length;
            p = new Point4[l];
            for (int i = points.Length - 1; i >= 0; i--)
            {
                p[i] = points[i];
            }
        }

        public override void Scale(float scalar)
        {
            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i].point *= scalar;
                if (points[i].useBackHandle) points[i].backHandle *= scalar;
                if (points[i].useForeHandle) points[i].foreHandle *= scalar;
            }
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = true;
        }

        public override void Normalize()
        {
            GetMinMax();
            Float4 offset = smoothMin; offset *= -1;
            Float4 scalar = smoothMax - smoothMin;
            if (scalar.x == 0) { scalar.x = 1; } else { scalar.x = 1f / scalar.x; }
            if (scalar.y == 0) { scalar.y = 1; } else { scalar.y = 1f / scalar.y; }
            if (scalar.z == 0) { scalar.z = 1; } else { scalar.z = 1f / scalar.z; }
            if (scalar.w == 0) { scalar.w = 1; } else { scalar.w = 1f / scalar.w; }

            for (int i = points.Length - 1; i >= 0; i--)
            {
                points[i].point += offset;
                points[i].point *= scalar;
                if (points[i].useBackHandle) points[i].backHandle *= scalar;
                if (points[i].useForeHandle) points[i].foreHandle *= scalar;
            }
            splineLength = GetTotalSplineLength(0.1f);
            anchorsDirty = true;
        }

        public void SetXOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.x = vals[i];
            }
        }

        public void SetYOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.y = vals[i];
            }
        }

        public void SetZOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.z = vals[i];
            }
        }

        public void SetWOnPoints(float[] vals)
        {
            int l = vals.Length; if (points.Length < l) { l = points.Length; }
            for (int i = 0; i < l; i++)
            {
                points[i].point.w = vals[i];
            }
        }

        public void AddPoints(Float4[] p, CurveType[] curves, float[] weights, int id = -1)
        {
            //Revamp this to do all the adding work itself, instead of repeatedly calling AddPoint.
            int npc = points.Length;
            int fillerSlots = 0;
            if (id > npc) { fillerSlots = id - npc; }

            totalWeight = 0;
            List<Point4> newPoints = new List<Point4>(points);
            List<SplineSection> newSS = new List<SplineSection>(sections);

            for (int i = 0; i < p.Length; i++)
            {

                CurveType curve = curves[i];
                float weight = weights[i];
                if (curves[i] == CurveType.None)
                {
                    curve = defaultCurveType;
                }
                if (weights[i] < 0)
                {
                    weight = defaultWeight;
                }

                if (id < 0)
                {
                    Point4 p2 = new Point4(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.weight = weight;
                    ss.curveType = curve;
                    newPoints.Add(p2); newSS.Add(ss);
                    totalWeight += weight;

                }
                else if (id < points.Length)
                {
                    Point4 p2 = new Point4(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.curveType = curve;
                    newPoints.Insert(id, p2); newSS.Insert(id, ss);
                    totalWeight += weight;

                }
                else
                {
                    for (int j = 0; j < fillerSlots; j++)
                    {
                        SplineSection se = new SplineSection();
                        se.weight = defaultWeight;
                        se.curveType = defaultCurveType;
                        newPoints.Add(new Point4()); newSS.Add(se);
                        totalWeight += defaultWeight;
                    }
                    Point4 p2 = new Point4(); p2.point = p[i];
                    SplineSection ss = new SplineSection();
                    ss.weight = weight;
                    ss.curveType = curve;
                    newPoints.Add(p2); newSS.Add(ss);
                }
            }
            points = newPoints.ToArray();
            sections = newSS.ToArray();

            anchorsDirty = true;
            extentsDirty = true;
            SetCapPoints();
            GetTotalWeight();
            GetTotalSplineLength(1);
        }

        public void AddPoint(Float4 p, int id = -1, CurveType s = CurveType.None, float t = -1f)
        {
            if (s == CurveType.None)
            {
                s = defaultCurveType;
            }
            if (t < 0)
            {
                t = defaultWeight;
            }


            int npc = points.Length;
            int fillerSlots = 0;
            if (id > npc) { fillerSlots = id - npc; }

            totalWeight = 0;
            List<Point4> newPoints = new List<Point4>(points);
            List<SplineSection> newSS = new List<SplineSection>(sections);

            if (id < 0)
            {
                Point4 p2 = new Point4(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.curveType = s;
                ss.weight = t;
                newPoints.Add(p2); newSS.Add(ss);
                totalWeight += t;

            }
            else if (id < points.Length)
            {
                Point4 p2 = new Point4(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.weight = t;
                ss.curveType = s;
                newPoints.Insert(id, p2); newSS.Insert(id, ss);
                totalWeight += t;

            }
            else
            {
                for (int i = 0; i < fillerSlots; i++)
                {
                    SplineSection se = new SplineSection();
                    se.weight = defaultWeight;
                    se.curveType = defaultCurveType;
                    newPoints.Add(new Point4()); newSS.Add(se);
                    totalWeight += defaultWeight;
                }
                Point4 p2 = new Point4(); p2.point = p;
                SplineSection ss = new SplineSection();
                ss.weight = t;
                ss.curveType = s;
                newPoints.Add(p2); newSS.Add(ss);
            }


            points = newPoints.ToArray();

            sections = newSS.ToArray();

            anchorsDirty = true;
            extentsDirty = true;
            SetCapPoints();
            GetTotalWeight();
            GetTotalSplineLength(1);
        }

        public override void RemovePoint(int id)
        {

            if (id >= 0 && id < points.Length)
            {
                List<Point4> newPoints = new List<Point4>(points);

                List<SplineSection> newSections = new List<SplineSection>(sections);

                newPoints.RemoveAt(id);
                if (id < points.Length - 1)
                {
                    newSections.RemoveAt(id);
                }
                points = newPoints.ToArray();

                sections = newSections.ToArray();

                totalWeight = 0;
                for (int i = sections.Length - 1; i >= 0; i--)
                {
                    totalWeight += sections[i].weight;
                }
            }
        }

        public override void RemovePoints(int[] ids)
        {
            List<Point4> newPoints = new List<Point4>(points);
            List<SplineSection> newSections = new List<SplineSection>(sections);

            for (int i = Length - 1; i >= 0; i--)
            {

                for (int j = 0; j < ids.Length; j++)
                {
                    if (i == ids[j])
                    {
                        newPoints.RemoveAt(ids[j]);
                        if (i < points.Length - 1) newSections.RemoveAt(ids[j]);
                    }
                }

            }

            points = newPoints.ToArray();

            sections = newSections.ToArray();

            totalWeight = 0;
            for (int i = sections.Length - 1; i >= 0; i--)
            {
                totalWeight += sections[i].weight;
            }
        }

        public override void SetCapPoints()
        {
            //First, how many points do we actually have?
            int l = points.Length;

            //We need at least two points.
            if (l > 1)
            {
                //Our starting control point is the difference between point 0 and point 1 subtracted from point 0.
                Float4 pointDiff = points[1].point; pointDiff -= points[0].point;
                startCap = points[0].point; startCap -= pointDiff;

                if (l >= 3)
                {
                    //We check to see if this is a loop - if the first and last official points are the same.
                    // If so, we set our control points to form that perfect, continuous loop.
                    if (points[0].point.Equals(points[l - 1].point))
                    {
                        startCap = points[l - 2].point;
                        endCap = points[1].point;
                    }
                    //If not, we create our two control points. Each is just a straight line from our second set of points, going inward, through our first 
                    //set of points.	
                    else
                    {
                        pointDiff = points[l - 1].point - points[l - 2].point;
                        endCap = points[l - 1].point; endCap += pointDiff;
                    }
                }
                else
                {
                    endCap = points[1].point; endCap += pointDiff;
                }
            }
        }

        public Float4 GetPoint(float percent, bool weighted = true)
        {

            Float4 currentPoint;
            if (points.Length > 0)
            {
                if (points.Length > 1)
                {
                    if (percent <= 0)
                    {
                        currentPoint = points[0].point;
                    }
                    else if (percent >= 1)
                    {
                        currentPoint = points[points.Length - 1].point;
                    }
                    else
                    {
                        int cS = GetSectionAtPercent(percent, weighted);

                        if (cS != currentSection)
                        {
                            currentSection = cS;
                            GetControlPoints();
                        }

                        Float4 point = Float4.zero;

                        switch (sections[cS].curveType)
                        {
                            //Catmull Rom
                            case CurveType.CatmullRom:
                                point.x = Splines.CatmullRom(weightedPercent, a.x, b.x, c.x, d.x);
                                point.y = Splines.CatmullRom(weightedPercent, a.y, b.y, c.y, d.y);
                                point.z = Splines.CatmullRom(weightedPercent, a.z, b.z, c.z, d.z);
                                point.w = Splines.CatmullRom(weightedPercent, a.w, b.w, c.w, d.w);
                                break;

                            //Linear
                            case CurveType.Linear:
                                point = Float4.Lerp(points[cS].point, points[cS + 1].point, weightedPercent);

                                break;

                            //CubicBezier
                            case CurveType.CubicBezier:
                                point.x = Splines.CubicBezier(weightedPercent, a.x, b.x, c.x, d.x);
                                point.y = Splines.CubicBezier(weightedPercent, a.y, b.y, c.y, d.y);
                                point.z = Splines.CubicBezier(weightedPercent, a.z, b.z, c.z, d.z);
                                point.w = Splines.CubicBezier(weightedPercent, a.w, b.w, c.w, d.w);
                                break;

                            //Step
                            case CurveType.Step:
                                point = points[cS].point;
                                break;
                        }
                        currentPoint = point;
                    }

                }
                else
                {
                    currentPoint = points[0].point;
                }
            }
            else
            {
                currentPoint = Float4.zero;
            }
            return currentPoint;
        }

        public override void GetControlPoints()
        {
            int l = sections.Length;
            if (currentSection == 0 && l > 1)
            {
                a = startCap;
                b = points[0].point;
                c = points[1].point;
                d = points[2].point;
            }
            else if (currentSection == 0)
            {
                a = startCap;
                b = points[0].point;
                c = points[1].point;
                d = endCap;
            }
            else if (currentSection == l - 1)
            {
                a = points[l - 2].point;
                b = points[l - 1].point;
                c = points[l].point;
                d = endCap;
            }
            else if (currentSection > 0)
            {
                a = points[currentSection - 1].point;
                b = points[currentSection].point;
                c = points[currentSection + 1].point;
                d = points[currentSection + 2].point;
            }

            if (points[currentSection].useBackHandle)
            {
                a = points[currentSection].point + points[currentSection].backHandle;
            }
            if (points[currentSection + 1].useForeHandle)
            {
                d = points[currentSection + 1].point + points[currentSection + 1].foreHandle;
            }
        }

        public Spline4 SplitSpline(int id, bool includeCurrent)
        {

            if (points.Length > id)
            {
                List<Point4> newPoints = new List<Point4>(points);

                List<SplineSection> newSections = new List<SplineSection>(sections);

                int diff = Length - id;
                List<Point4> splitPoints = newPoints.GetRange(id, diff);
                List<SplineSection> splitSections = newSections.GetRange(id, diff);


                points = newPoints.GetRange(0, id - 1).ToArray();
                sections = newSections.GetRange(0, id - 1).ToArray();

                totalWeight = 0;
                for (int i = sections.Length - 1; i >= 0; i--)
                {
                    totalWeight += sections[i].weight;
                }

                Spline4 leftOvers = new Spline4();
                leftOvers.SetAnchorPoints(splitPoints.ToArray());
                leftOvers.sections = splitSections.ToArray();

                leftOvers.totalWeight = 0;
                for (int i = leftOvers.sections.Length - 1; i >= 0; i--)
                {
                    leftOvers.totalWeight += leftOvers.sections[i].weight;
                }
                return leftOvers;
            }
            else
            {
                return new Spline4();
            }
        }

        public Spline4 GetSubSpline(int start, int end)
        {
            List<Point4> newPoints = new List<Point4>(points);

            List<SplineSection> newSections = new List<SplineSection>(sections);

            int diff = end - start;
            int ddif = diff - 1;
            Spline4 s = new Spline4();
            if (start > end)
            {
                if (!wrap) { return null; }
                //diff = points.Length - 1 - start;
                diff = points.Length - start;
                ddif = diff - 1;
                newPoints = newPoints.GetRange(start, diff);
                newSections = newSections.GetRange(start, ddif);

                for (int i = 1; i <= end; i++)
                {
                    newPoints.Add(points[i]);

                    newSections.Add(sections[i]);

                }
            }
            else
            {
                newPoints = newPoints.GetRange(start, diff);
                newSections = newSections.GetRange(start, ddif);
            }

            s.points = newPoints.ToArray();
            s.sections = newSections.ToArray();

            if (start > 0)
            {
                s.startCap = points[start - 1].point;

            }
            else { s.startCap = startCap; }
            if (end < points.Length - 1)
            {
                s.endCap = points[end + 1].point;

            }
            else { s.endCap = new Float4(endCap); }

            s.anchorsDirty = true;

            s.Sections = newSections.Count;
            s.GetTotalWeight();
            s.GetTotalSplineLength(0.01f, true);
            return s;
        }

        //I should get either ClosestPolyPoint or this working for both circumstances. Start by checking to see which works better.
        public int GetClosestAnchorPoint(Float4 p)
        {
            float lowestDist = float.PositiveInfinity;
            int id = -1;
            for (int i = Length - 1; i >= 0; i--)
            {

                float d = Float4.Distance(p, this[i]);
                if (d < lowestDist)
                {
                    id = i;
                    lowestDist = d;
                }
            }
            return id;
        }

        public Float4 ClosestPointOnSpline(Float4 point, int samples, ref int closestAnchor, bool weighted = true)
        {
            closestAnchor = points.Length - 2;
            int nextClosestAnchor = -1;

            float currentDistance = Float4.Distance(point, points[points.Length - 1].point),
                currentPer = 1f, per, nextPer;

            Float4 p1, p2 = points[points.Length - 2].point, p3 = points[points.Length - 1].point;

            nextPer = 1;

            for (int i = points.Length - 2; i >= 0; i--)
            {
                per = GetPercentAtPoint(i, weighted);
                float incr = (nextPer - per) / samples;

                for (int j = samples - 1; j >= 0; j--)
                {
                    Float4 tp2 = GetPoint(per + (incr * j));
                    float dist = Float4.Distance(point, tp2);
                    if (dist < currentDistance)
                    {
                        p2 = tp2;
                        currentDistance = dist;
                        currentPer = per + (incr * j);
                        nextClosestAnchor = closestAnchor;
                        closestAnchor = i;
                    }
                }
                nextPer = per;
            }

            p1 = points[closestAnchor].point;
            if (nextClosestAnchor > closestAnchor)
            {
                p3 = points[closestAnchor + 1].point;
            }
            else
            {
                p3 = points[closestAnchor - 1].point;
            }

            per = GetPercentAtPoint(closestAnchor, weighted);
            currentPer = (nextPer + per) * 0.5f;
            p2 = GetPoint(currentPer, weighted);

            for (int i = samples; i > 0; i--)
            {
                float dist1 = Float4.Distance(p1, point),
                      dist2 = Float4.Distance(p3, point);
                if (dist1 < dist2)
                {
                    p3 = p2;
                    nextPer = currentPer;
                    currentPer = (per + currentPer) * 0.5f;
                }
                else
                {
                    p1 = p2;
                    per = currentPer;
                }
                currentPer = (nextPer + per) * 0.5f;
                p2 = GetPoint(currentPer, weighted);
            }
            return p2;
        }

        public override float GetTotalSplineLength(float percent, bool resetHeldValue = false)
        {

            float sLength = 0f;

            if (sections.Length > 0)
            {

                float secPer = 0;
                float sp = 0;
                for (int j = 0; j < sections.Length; j++)
                {
                    sections[j].length = 0;
                    if (sections[j].curveType == CurveType.Linear)
                    {
                        sections[j].length = Float4.Distance(points[j].point, points[j + 1].point);
                        sLength += sections[j].length;
                    }
                    else if (sections[j].curveType == CurveType.CatmullRom || sections[j].curveType == CurveType.CubicBezier)
                    {

                        Float4 a = points[j].point, b;
                        secPer += sections[j].weight;
                        float isp = secPer / totalWeight;

                        for (float i = percent; i <= 1; i += percent)
                        {
                            b = GetPoint(Float.Lerp(sp, isp, i));
                            float d = Float4.Distance(a, b);
                            a = b;
                            sections[j].length += d;
                            sLength += d;
                        }
                        sp = isp;
                    }
                }
            }
            if (resetHeldValue) { splineLength = sLength; }
            return sLength;
        }

        public Float4 AveragePoints(bool useSmoothed = true)
        {

            Float4 average = Float4.zero;

            if (useSmoothed)
            {
                Float4[] pts = SmoothedPoints;
                for (int i = 0; i < pts.Length; i++)
                {
                    average += pts[i];
                }
                average /= pts.Length;
            }
            else
            {
                for (int i = 0; i < points.Length; i++)
                {
                    average += points[i].point;
                }
                average /= points.Length;
            }

            return average;
        }

        public void Offset(Float4 offset)
        {
            for (int i = Length - 1; i >= 0; i--)
            {
                this[i] += offset;
            }
            anchorsDirty = true;
            extentsDirty = true;
        }

        public override void ResamplePoints(int samples)
        {
            if (samples > 2)
            {

                float perc = 1f / (samples - 1);

                Point4[] newPoints = new Point4[samples];
                SplineSection[] secs = new SplineSection[samples - 1];

                newPoints[0] = points[0];
                secs[0] = sections[0];

                int s = 0;
                for (int i = 1; i < points.Length; i++)
                {
                    float ip = GetPercentAtPoint(i), sp = s * perc;
                    while (sp < ip)
                    {
                        s++;
                        newPoints[s] = points[i];
                        newPoints[s].point = GetPoint(sp);
                        if (i < points.Length - 1) { secs[s] = sections[i]; }
                        sp = s * perc;
                    }
                    if (s == samples) { break; }
                }

                points = newPoints;
                sections = secs;
            }
        }

        public override float Distance(float p1, float p2)
        {
            return Distance(p1, p2, 20);
        }

        public override float Distance(float p1, float p2, int samples)
        {
            float distance = 0f;
            float p = p2 - p1 / samples;
            float cP = p1;
            Float4 currentPoint = GetPoint(p1);
            Float4 prevPoint = currentPoint;

            for (int i = samples - 1; i >= 0; i--)
            {
                cP += p;
                currentPoint = GetPoint(cP);
                distance += Float4.Distance(prevPoint, currentPoint);
                prevPoint = currentPoint;
            }
            return distance;
        }

        public Float4[] GetSmoothedPoints(out int[] newIDS, int maxSubdivisions = 10, float subDivideAngle = 15)
        {
            if (maxSubdivisions < 3) { maxSubdivisions = 3; }
            //Debug.Log("smoothing " + name);
            List<Float4> newPoints = new List<Float4>();
            List<int> nIDS = new List<int>();
            //Debug.Log((points.Length - 2) + " | " + sectionTypes.Length);
            for (int i = 0; i < points.Length - 1; i++)
            {
                nIDS.Add(newPoints.Count);
                newPoints.Add(points[i].point);
                //Debug.Log("Switching section type " + i);
                //Debug.Log("Total Weight:" + totalWeight);
                switch (sections[i].curveType)
                {
                    case CurveType.CatmullRom:
                    case CurveType.CubicBezier:

                        currentSection = -1;
                        float pa = GetPercentAtPoint(i), pb = GetPercentAtPoint(i + 1);
                        //Debug.Log(i + ". PA: " + pa + "  PB: " + pb);
                        float diff = pb - pa;
                        float per = diff / maxSubdivisions;

                        for (int j = 1; j < maxSubdivisions; j++)
                        {
                            newPoints.Add(GetPoint(pa + (per * j)));
                        }

                        break;
                }
            }
            nIDS.Add(newPoints.Count);
            newPoints.Add(points[points.Length - 1].point);

            newIDS = nIDS.ToArray();
            return newPoints.ToArray();
        }

        public override int GetSectionID(int smoothID)
        {

            int currentSec = newIDS[points.Length - 1];
            if (smoothID <= currentSec)
            {
                for (int i = points.Length - 2; i >= 0; i--)
                {
                    if (smoothID < currentSec)
                    {
                        return i;
                    }
                    currentSec = newIDS[i];
                }
                return points.Length - 1;
            }
            return -1;
        }

        public void RotateSpline(Float3 angle)
        {
            Float4 ec = extents.Center;
            Float3 c = new Float3(ec.x, ec.y, ec.z);
            for (int i = points.Length - 1; i >= 0; i--)
            {
                Float3 p = new Float3(points[i].point.x, points[i].point.y, points[i].point.z);
                p = p.RotateAround(c, angle);
                points[i].point = new Float4(p.x, p.y, p.z, points[i].point.w);
            }
        }

        public void ScaleSpline(Float3 scale)
        {
            Float4 ec = extents.Center;
            Float3 c = new Float3(ec.x, ec.y, ec.z);
            for (int i = points.Length - 1; i >= 0; i--)
            {
                Float3 p = new Float3(points[i].point.x, points[i].point.y, points[i].point.z);
                p -= c;
                p *= scale;
                p += c;
                points[i].point = new Float4(p.x, p.y, p.z, points[i].point.w);
            }
        }

        #endregion

    }

}
