using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using ProceduralToolkit;
using ProceduralToolkit.FastNoiseLib;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling;

public class ProceduralHeightmap : MonoBehaviour
{
    [Serializable]
    public class MapRegionFilter
    {
        public enum FilterType {LinearGradient, RadialGradient, EdgeGradient };
        public FilterType filterType;

        //public Gradient gradient;

        public Float2 gradientPoint, 
                      gradientDirection, 
                      perpendicularA, perpendicularB;

        public void GetPerpendiculars()
        {
            Float2 perpDir = Float2.Perpendicular(gradientDirection).Normalize();
            perpendicularA = gradientPoint + perpDir * 10000f;
            perpendicularB = gradientPoint - perpDir * 10000f;
        }
    }

    [Serializable]
    public class MapRegion
    {
        public string name;
        public Spline2 regionBounds;
        public FastNoise noise;
        public FastNoise.NoiseType noiseType;
        public int seed;
        public FastNoise.Interp noiseInterpolation;
        public FastNoise.FractalType noiseFractal;

        public MapRegionFilter[] filters = new MapRegionFilter[0];

        public MapRegion()
        {
            regionBounds = Spline2.NewPolygon(4, 1.5f, -45f);
        }

        public MapRegion(string name)
        {
            this.name = name;
            regionBounds = Spline2.NewPolygon(4, 1.5f, 45f);
            regionBounds.name = name + "_RegionBounds";
        }

        public MapRegion(string name, Vector2Int regionSize)
        {
            this.name = name;
            regionSize.x--;
            regionSize.y--;
            regionBounds = Spline2.NewPolygon(4, 1.5f);
            regionBounds[0] = Float2.zero;
            regionBounds[1] = new Float2(0, regionSize.y);
            regionBounds[2] = new Float2(regionSize.x, regionSize.y);
            regionBounds[3] = new Float2(regionSize.x, 0);
        }
        public void Enable()
        {
            if (noise != null) return;
            regionBounds.extentsDirty = true;
            regionBounds.anchorsDirty = true;
            var e =regionBounds.Extents;
            var s = regionBounds.SmoothedPoints;

            noise = new FastNoise(seed);
            noise.SetNoiseType(noiseType);
            noise.SetFractalType(noiseFractal);
            noise.SetInterp(noiseInterpolation);
        }

        public float GetNoise(Float2 uv)
        {
            float value = 0;
            if (regionBounds.ContainsPoint(uv))
            {
                value = noise.GetNoise01(uv.x, uv.y);
                for(int i =0; i < filters.Length; i++)
                {
                    MapRegionFilter filter = filters[i];
                    Float mag = filter.gradientDirection.Magnitude();
                    switch (filter.filterType)
                    {
                        case MapRegionFilter.FilterType.LinearGradient:
                            
                            Float2 gd = filter.gradientDirection.Normalize();

                            Float2 lA = uv + gd * 10000f;
                            Float2 lB = uv - gd * 10000f;

                            Float2 intersect = Float2.zero;
                            if( Float2.LineIntersection(filter.perpendicularA, filter.perpendicularB, lA, lB, ref intersect))
                            {
                                float linearValue = 0.5f - Float2.Distance(intersect, uv) * mag;
                                value *= linearValue;
                            }

                            break;

                        case MapRegionFilter.FilterType.RadialGradient:

                            float distanceFromCenter = Float2.Distance(filter.gradientPoint, uv);
                            float radialValue = 0.5f - distanceFromCenter * mag;
                            value *= radialValue;

                            break;

                        case MapRegionFilter.FilterType.EdgeGradient:
                            int closestPointID = -1;
                            Float2 closestPoint = regionBounds.GetClosestPointOnPolygon(uv, 10, ref closestPointID);
                            float distanceFromEdge = Float2.Distance(closestPoint, uv);
                            float edgeValue = 0.5f - distanceFromEdge * mag;
                            value *= edgeValue;
                            break;
                    }
                }
            }

            return value;
        }
    }

    public MapRegion[] regions = new MapRegion[0];
    public Vector2Int heightmapSize;

    [SerializeField]
    protected Texture2D heightmap;

    protected bool heightmapDirty;
    public Texture2D Heightmap
    {
        get
        {
            if (heightmapDirty)
            {
                GenerateHeightmap();
            }
            return heightmap;
        }
    }

    public virtual void UpdateHeightmap()
    {
        heightmapDirty = true; GenerateHeightmap();
    }

    protected virtual void GenerateHeightmap()
    {
        for(int i =0; i < regions.Length; i++)
        {
            regions[i].Enable();
        }

        if(heightmap != null)
        {
            if(heightmap.width != heightmapSize.x || heightmap.height != heightmapSize.y)
            {
                IdeamUnity.Destroy(heightmap);
            }
        }

        if(heightmap == null)
        heightmap = new Texture2D(heightmapSize.x, heightmapSize.y, TextureFormat.ARGB32, false, true)
        {
            filterMode = FilterMode.Point,
            name = gameObject.name + "_Heightmap"
        };

        
        float[] values = new float[heightmapSize.x * heightmapSize.y];
        Color[] pixels = new Color[heightmapSize.x * heightmapSize.y];

        //float uScalar = 1f / heightmapSize.x;
        //float vScalar = 1f / heightmapSize.y;
        float min = 0, max = 0;
        for (int x = 0; x < heightmapSize.x; x++)
        {
            float u = x;// * uScalar;

            for (int y = 0; y < heightmapSize.y; y++)
            {
                float v = y;// * vScalar;
                Float2 uv = new Float2(u, v);
                float value = 0f;

                for(int r = 0; r < regions.Length; r++)
                {
                    float regionValue = regions[r].GetNoise(uv);
                    value += regionValue;
                }

                int yy = heightmapSize.y - y -1;

                values[yy * heightmapSize.x + x] = value;

                if(value < min) { min = value; }
                if(value > max) { max = value; }
            }
        }

        float size = max - min;
        float scalar = 1f / size;

        for(int i=0; i < values.Length; i++)
        {
            float value = values[i] * scalar;
            pixels[i] = new Color(value, value, value, 1f);
        }
        heightmap.SetPixels(pixels);

        heightmap.Apply();

        heightmapDirty = false;
    }

    public int NewRegion()
    {
        int id = regions.Length;
        System.Array.Resize(ref regions, id + 1);
        regions[id] = new MapRegion("Region " + (id + 1).ToString(), heightmapSize);

        return id;
    }
}
