using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Applied.Layout;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.Science.Applied.Layout
{
    public class GameObjectRadialLayout : GameObjectLayout, IRadialLayout
    {

        [SerializeField]
        protected Float3 normalDirection;
        public Float3 NormalDirection { get { return normalDirection; } set { if (value != normalDirection) { normalDirection = value; PerformLayout(); } } }

        [SerializeField]
        protected float radius;
        public float Radius { get { return radius; } set { if (value != radius) { radius = value; PerformLayout(); } } }

        public override void PerformLayout()
        {
        }
    }
}
