using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;

namespace Ideam.Science.Applied.Cartography.Maps
{
    public interface IGridBasedMap<TGrid, TSymmetry> : IMap
        where TGrid : ISymmetryGroup<TSymmetry>
        where TSymmetry : ISymmetry
    {
        TGrid Grid { get; }
        int[] GetGridFootprint<T>(T feature) where T : IFeature;
    }
}
