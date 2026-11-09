using System;
using System.Collections;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    [System.Serializable]
    public class C2MM : SymmetryGroup<C2MM,Symmetry3>
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
                return Topology.RightTriangle();
            }
        }

        public C2MM() : base() { }
        public C2MM(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }

        public override Symmetry3 GetNeighbor(Symmetry3 baseCell, int cellID, int neighborID)
        {

            Symmetry3 newNeighbor = baseCell;
            int[] neighbors = new int[] { -1, -1, -1 };
            Float2 position = newNeighbor.Position, scale = newNeighbor.Scale;
            switch (neighborID)
            {
                case 0:
                    if (scale.y >= 0)
                    {
                        position.x++;
                    }
                    else
                    {
                        position.x--;
                    }
                    scale.x *= -1;
                    neighbors[0] = cellID;
                    break;
                case 1:
                    if (scale.x >= 0)
                    {
                        position.y--;
                    }
                    else
                    {
                        position.y++;
                    }
                    scale.y *= -1;
                    neighbors[1] = cellID;
                    break;
                case 2:
                    if (newNeighbor.Angle >= 0)
                    {
                        newNeighbor.Angle += 180;
                    }
                    else
                    {
                        newNeighbor.Angle -= 180;
                    }
                    neighbors[2] = cellID;
                    break;
            }
            newNeighbor.Position = position;
            newNeighbor.Scale = scale;
            newNeighbor.Neighbors = neighbors;
            return newNeighbor;
        }

        public override void Setup()
        {
            symmetryTemplate = new Symmetry3(Float2.zero, Float2.one, 0f);
        }
    }
    
}

