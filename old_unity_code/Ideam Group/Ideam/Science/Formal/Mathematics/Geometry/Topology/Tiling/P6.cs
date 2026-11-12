using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    [System.Serializable]
    public class P6 : HexagonalSymmetryGroup<P6, Symmetry3>
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
                if (rotationTolerance < 1f) { rotationTolerance = 1f; }
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
                return Topology.FlatTriangle();
            }
        }

        public P6() : base() { }
        public P6(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }

        public override Symmetry3 GetNeighbor(Symmetry3 baseCell, int cellID, int neighborID)
        {

            Symmetry3 newNeighbor = baseCell;
            int[] neighbors = new int[] { -1, -1, -1 };
            Float2 position = newNeighbor.Position;
            switch (neighborID)
            {
                case 0: 
                    RotateAround(ref newNeighbor, Float2.down * 0.43301f, 180);
                    neighbors[0] = cellID;
                    break;
                case 1:
                    RotateAround(ref newNeighbor, Float2.up * 0.43301f, 60);
                    neighbors[2] = cellID;
                    break;
                case 2:
                    RotateAround(ref newNeighbor, Float2.up * 0.43301f, -60);
                    neighbors[1] = cellID;
                    break;
            }

            newNeighbor.Neighbors = neighbors;
            return newNeighbor;
        }

        public override void Setup()
        {
            symmetryTemplate = new Symmetry3(Float2.zero, Float2.one, 0f);
        }

        public override IEnumerator Hexize()
        {
            int[] vertCounts = new int[vertices.Length];

            List<int> hexVerts = new List<int>();

            int l = symmetries.Length;

            for (int i = 0; i < l; i++)
            {
                vertCounts[symmetries[i].vert2]++;
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
                if (vertCounts[symmetries[i].vert2] < 3)
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

