using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;

namespace Ideam.Science.Applied.Layout
{
    public interface ILinearLayout : ILayout
    {
        Float3 Direction { get; set; }   
    }
}