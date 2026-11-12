using System;
using System.Collections;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    [System.Serializable]
    public class PG : SymmetryGroup<PG, Symmetry6>
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
                return Topology.Brick();
            }
        }

        public PG() : base() { }
        public PG(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }

        public override Symmetry6 GetNeighbor(Symmetry6 baseCell, int cellID, int neighborID)
        {

            Symmetry6 newNeighbor = baseCell;
            int[] neighbors = new int[] { -1, -1, -1, -1, -1, -1 };
            Float2 position = newNeighbor.Position;
            switch (neighborID)
            {
                case 0:
                    position.x += 0.5f;
                    position.y++;
                    neighbors[3] = cellID;
                    break;
                case 1:
                    position.x -= 0.5f;
                    position.y++;
                    neighbors[2] = cellID;
                    break;
                case 2:
                    position.x += 0.5f;
                    position.y--;
                    neighbors[1] = cellID;
                    break;
                case 3:
                    position.x -= 0.5f;
                    position.y--;
                    neighbors[0] = cellID;
                    break;

                case 4:
                    position.x++;
                    neighbors[5] = cellID;
                    break;

                case 5:
                    position.x--;
                    neighbors[4] = cellID;
                    break;
                    
            }

            newNeighbor.Position = position;
            newNeighbor.Neighbors = neighbors;
            return newNeighbor;
        }

        public override void Setup()
        {
            symmetryTemplate = new Symmetry6(Float2.zero, Float2.one, 0f);
        }
    }
    
}

