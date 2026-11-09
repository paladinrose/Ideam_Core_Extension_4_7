using System;
using System.Collections;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    
    [System.Serializable]
    public class P1 : SymmetryGroup<P1, Symmetry4>
    {
        
        float positionTolerance = 0.01f, rotationTolerance = 1f, scaleTolerance = 0.01f;

        public override float PositionTolerance
        {
            get { return positionTolerance; }

            set
            {
                positionTolerance = value;
                if(positionTolerance < 0.01f) { positionTolerance = 0.01f; }
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
                return Topology.Quad();
            }
        }
        
        public P1() : base() { }
        public P1(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }

        public override Symmetry4 GetNeighbor(Symmetry4 baseCell, int cellID, int neighborID)
        {

            Symmetry4 newNeighbor = baseCell;
            int[] neighbors = new int[] { -1, -1, -1, -1 };
            Float2 position = newNeighbor.Position;
            switch (neighborID)
            {
                case 0:
                    position.x += 1;
                    neighbors[1] = cellID;
                    break;
                case 1:
                    position.x -= 1;
                    neighbors[0] = cellID;
                    break;
                case 2:
                    position.y += 1;
                    neighbors[3] = cellID;
                    break;
                case 3:
                    position.y -= 1;
                    neighbors[2] = cellID;
                    break;
            }
            newNeighbor.Position = position;
            newNeighbor.Neighbors = neighbors;
            return newNeighbor;
        }

        public override void Setup()
        {
            symmetryTemplate = new Symmetry4(Float2.zero, Float2.one, 0f);
        }
    }
    
}

