using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Topology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class SymmetryGroupGameBoard : UnityGameBoard
    {
        public ComplexSymmetryGroup group;

        public override void BoardAwake()
        {
            base.BoardAwake();

            StartCoroutine(BuildGroupSpaces());
        }

        IEnumerator BuildGroupSpaces()
        {

            yield break;
        }
    }
}