using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    [System.Serializable]
    public class P3 : HexagonalSymmetryGroup<P3, Symmetry4>
    {
        
        float positionTolerance = 0.01f, rotationTolerance = 1f, scaleTolerance = 0.01f;

        public override float PositionTolerance
        {
            get { return positionTolerance; }

            set
            {
                positionTolerance = value;
                if (positionTolerance < 0.01f) { positionTolerance = 0.01f; }
            }
        }

        public override float RotationTolerance
        {
            get { return rotationTolerance; }

            set
            {
                rotationTolerance = value;
                if (rotationTolerance < 61f) { rotationTolerance = 1f; }
            }
        }

        public override float ScaleTolerance
        {
            get { return scaleTolerance; }

            set
            {
                scaleTolerance = value;
                if (scaleTolerance < 0.01f) { scaleTolerance = 0.01f; }
            }
        }

        public override Float2[] PrimitiveVertices
        {
            get
            {
                return Topology.Rhombus();
            }
        }

        public P3() : base() { }

        public P3(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        {  }

        public override Symmetry4 GetNeighbor(Symmetry4 baseCell, int cellID, int neighborID)
        {
            Symmetry4 newNeighbor = new Symmetry4(baseCell);

            int[] neighbors = new int[] { -1, -1, -1, -1 };
            newNeighbor.Neighbors = neighbors;
            
            switch (neighborID)
            {
                case 0:
                    RotateAround(ref newNeighbor, new Float2(-0.5f, -0.2886f), 120, true);
                    break;
                case 1:
                    RotateAround(ref newNeighbor, new Float2(-0.1667f, 0.2886f), 120, true);
                    break;
                case 2:
                    RotateAround(ref newNeighbor, new Float2(0.5f, 0.2886f), 120, true);
                    break;
                case 3:
                    RotateAround(ref newNeighbor, new Float2(0.1667f, -0.2886f), 120, true);
                    break;
            }
            return newNeighbor;
        }

        public override void Setup()
        {
            symmetryTemplate = new Symmetry4(Float2.zero, Float2.one, 0f);
        }

        public override IEnumerator Hexize()
        {
            int[] vertCounts = new int[vertices.Length];

            List<int> hexVerts = new List<int>();

            int l = symmetries.Length;

            for (int i = 0; i < l; i++)
            {
                vertCounts[symmetries[i].vert4]++;
            }

            for (int i = 0; i < vertCounts.Length; i++)
            {
                if (vertCounts[i] == 3)
                {
                    hexVerts.Add(i);
                }
            }

            hexCenters = hexVerts.ToArray();
            /*
            List<int> toRemove = new List<int>();
            for (int i = 0; i < l; i++)
            {
                if (vertCounts[symmetries[i].vert4] < 3)
                {
                    toRemove.Add(i);
                }
            }

            RemoveSymmetries(toRemove.ToArray());
            */
            
            yield return null;

            IEnumerator getHexEdges = GetHexEdges();
            while (getHexEdges.MoveNext()) { yield return null; }
        }
    }
    
}

