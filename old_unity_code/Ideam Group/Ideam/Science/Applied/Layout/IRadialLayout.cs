using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;

namespace Ideam.Science.Applied.Layout
{
    public interface IRadialLayout : ILayout
    {
        Float3 NormalDirection { get; set; }
        float Radius { get; set; }
    }
}
