using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Applied.Layout;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;

namespace Ideam.Unity.Science.Applied.Layout
{
    public class GameObjectGridLayout : GameObjectLayout, IGridLayout
    {
        [SerializeField]
        protected SymmetryGroupType gridType;
        public SymmetryGroupType GridType { get { return gridType; } set { if(value != gridType) { gridType = value; PerformLayout(); } } }

        public override void PerformLayout()
        {
        }
    }
}