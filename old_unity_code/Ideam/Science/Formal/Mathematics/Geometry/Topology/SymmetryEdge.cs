using System;

using Ideam.Science.Formal.Graphs;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    public delegate void EdgeHandler(SymmetryEdge edge);
    
    [Serializable]
    public struct SymmetryEdge : IEdge
    {
        public int vertexA, vertexB, nodeA, nodeB;

        public int NodeA { get { return nodeA; } set { nodeA = value; } }
        public int PortA { get { return vertexA; } set { vertexA = value; } }

        public int NodeB { get { return nodeB; } set { nodeB = value; } }
        public int PortB { get { return vertexB; } set { vertexB = value; } }

        public bool isBorder;

        public byte edgeState;

        public float edgeLength, crossDistance;

        public SymmetryEdge(int a, int b)
        {
            vertexA = a; vertexB = b; isBorder = false;
            nodeA = nodeB = -1;
            edgeState = 0;
            edgeLength = -1; crossDistance = -1;
        }

        public SymmetryEdge(SymmetryEdge e)
        {
            vertexA = e.vertexA;
            vertexB = e.vertexB;
            nodeA = e.nodeA;
            nodeB = e.nodeB;
            isBorder = e.isBorder;
            edgeState = e.edgeState;
            edgeLength = e.edgeLength;
            crossDistance = e.crossDistance;
        }


        public override bool Equals(object obj)
        {
            SymmetryEdge other = (SymmetryEdge)obj;

            if ((vertexA == other.vertexA && vertexB == other.vertexB) || (vertexA == other.vertexB && vertexB == other.vertexA))
            {
                return true;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }
        public static bool operator ==(SymmetryEdge a, SymmetryEdge b)
        {
            if ((a.vertexA == b.vertexA && a.vertexB == b.vertexB) || (a.vertexA == b.vertexB && a.vertexB == b.vertexA))
            {
                return true;
            }
            return false;
        }

        public static bool operator !=(SymmetryEdge a, SymmetryEdge b)
        {
            return !(a == b);
        }
    }

}
