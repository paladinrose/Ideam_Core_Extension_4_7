using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;

namespace Ideam.Science.Applied.Cartography.Maps
{
    public abstract class AbstractGridMap<TGrid, TSymmetry> : IGridBasedMap<TGrid, TSymmetry>
        where TGrid : ISymmetryGroup<TSymmetry>
        where TSymmetry : ISymmetry
    {
        public TGrid grid;

        public virtual TGrid Grid { get { return grid; } }

        public abstract T[] GetFeatures<T>() where T : IFeature;

        public virtual int[] GetGridFootprint<T>(T feature) where T : IFeature
        {
            Float2[] featureFootprint = feature.CoordinatesFootprint<Float2>();
            List<int> gridCoords = new List<int>();
            for(int i = 0; i < featureFootprint.Length; i++)
            {
                int gridID = -1;
                if(grid.ContainsPoint(featureFootprint[i], ref gridID))
                {
                    if(!gridCoords.Contains(gridID))gridCoords.Add(gridID);
                }
            }
            return gridCoords.ToArray();
        }
    }
}