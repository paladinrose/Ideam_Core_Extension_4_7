using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    [System.Serializable]
    public class P6MM : HexagonalSymmetryGroup<P6MM, Symmetry3>
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
                return Topology.TallTriangle();
            }
        }

        public P6MM() : base() { }
        public P6MM(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }

        public override Symmetry3 GetNeighbor(Symmetry3 baseCell, int cellID, int neighborID)
        {

            Symmetry3 newNeighbor = baseCell;
            int[] neighbors = new int[] { -1, -1, -1 };
            Float2 position = newNeighbor.Position, scale = newNeighbor.Scale;
            switch (neighborID)
            {
                case 0:
                    float angle = 0;
                    if (scale.x >= 0)
                    {
                        angle = -60;
                    }
                    else
                    {
                        angle = 60;
                    }
                    if (scale.y < 0)
                    {
                        angle *= -1;
                    }
                    scale.x *= -1;
                    newNeighbor.Angle += angle;
                    break;

                case 1:
                    
                    Translate(ref newNeighbor, Float2.down, true);
                   
                    scale.y *= -1;
                    neighbors[1] = cellID;
                    break;

                case 2:

                    Translate(ref newNeighbor, Float2.right * 0.57735f, true);
                    
                    scale.x *= -1;
                    neighbors[2] = cellID;
                    break;
            }
             
            newNeighbor.Scale = scale;
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
                vertCounts[symmetries[i].vert1]++;
            }

            for (int i = 0; i < vertCounts.Length; i++)
            {
                if (vertCounts[i] == 12)
                {
                    hexVerts.Add(i);
                }
            }

            hexCenters = hexVerts.ToArray();
            /*
            List<int> toRemove = new List<int>();
            for (int i = 0; i < l; i++)
            {
                if (vertCounts[symmetries[i].vert1] < 12)
                {
                    toRemove.Add(i);
                }
            }

            RemoveSymmetries(toRemove.ToArray());
            */

            IEnumerator getHexEdges = GetHexEdges();
            while (getHexEdges.MoveNext()) { yield return null; }
        }
    }
    
}

