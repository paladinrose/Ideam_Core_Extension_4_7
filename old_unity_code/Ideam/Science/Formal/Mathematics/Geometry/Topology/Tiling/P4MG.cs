using System;
using System.Collections;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;


namespace Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling
{
    [System.Serializable]
    public class P4MG : SymmetryGroup<P4MG, Symmetry3>
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

        public P4MG() : base() { }
        public P4MG(Spline2[] region, Float2 start, Action<IEnumerator> buildAction) : base(region, start, buildAction)
        { }

        public override Symmetry3 GetNeighbor(Symmetry3 baseCell, int cellID, int neighborID)
        {

            Symmetry3 newNeighbor = baseCell;
            int[] neighbors = new int[] { -1, -1, -1 };
            Float2 position = newNeighbor.Position, scale = newNeighbor.Scale;
            switch (neighborID)
            {
                case 0:
                    //Rotate 90 degrees around -0.5, -0.5
                    RotateAround(ref newNeighbor, new Float2(-0.5f, -0.5f), 90, true);
                    //neighbors[1] = cellID;
                    break;
                case 1:
                    //Rotate 180 degrees around -0.5, 0.5
                    RotateAround(ref newNeighbor, new Float2(-0.5f, 0.5f), 180, true);
                    //neighbors[2] = cellID;
                    break;
                case 2:
                    //Glide reflect around 0,0
                    //position += new Float2(0.5f, 0.5f) * scale;
                    //This should maybe be x *= -1, or, failing that, both x and y *= -1
                    scale *= -1f;
                    newNeighbor.Scale = scale;
                    //newNeighbor.Position = position;
                    //neighbors[0] = cellID;
                    break;
               
            }

           
            newNeighbor.Neighbors = neighbors;
            return newNeighbor;
        }

        public override void Setup()
        {
            symmetryTemplate = new Symmetry3(Float2.zero, Float2.one, 0f);
        }
    }
    
}

