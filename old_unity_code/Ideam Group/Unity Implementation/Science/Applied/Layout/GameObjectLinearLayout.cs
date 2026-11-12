using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Applied.Layout;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.Science.Applied.Layout
{
    public class GameObjectLinearLayout : GameObjectLayout, ILinearLayout
    {
        [SerializeField]
        protected Float3 direction;
        public Float3 Direction { get { return direction; } set { if (value != direction) { direction = value; PerformLayout(); } } }

        public override void PerformLayout()
        {

        }
    }
}
