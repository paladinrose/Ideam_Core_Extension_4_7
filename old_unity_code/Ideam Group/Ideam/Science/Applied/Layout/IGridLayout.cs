using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Topology;
namespace Ideam.Science.Applied.Layout
{
    public interface IGridLayout : ILayout
    {
        SymmetryGroupType GridType { get; set; }
    }
}