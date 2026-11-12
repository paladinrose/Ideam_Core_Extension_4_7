using System;
using System.Collections;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    [System.Serializable]
    public class P4 : SymmetryGroup<P4, Symmetry4>
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
                return Topology.Quad();
            }
        }

        public P4() : base() { }
        public P4(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }

        public override Symmetry4 GetNeighbor(Symmetry4 baseCell, int cellID, int neighborID)
        {

            Symmetry4 newNeighbor = baseCell;
            int[] neighbors = new int[] { -1, -1, -1, -1 };
            Float2 position = newNeighbor.Position;

            switch (neighborID)
            {
                case 0:
                    RotateAround(ref newNeighbor, new Float2(0.5f, 0.5f), 90, true);
                    break;
                case 1:
                    RotateAround(ref newNeighbor, new Float2(0.5f, 0.5f), 180, true);
                    break;
                case 2:
                    RotateAround(ref newNeighbor, new Float2(0.5f, 0.5f), 270, true);
                    break;
                case 3:
                    RotateAround(ref newNeighbor, new Float2(0.5f, -0.5f), 180, true);
                    break;
            }
            
            newNeighbor.Neighbors = neighbors;
            return newNeighbor;
        }

        public override void Setup()
        {
            symmetryTemplate = new Symmetry4(Float2.zero, Float2.one, 0f);
        }
    }
    
}

