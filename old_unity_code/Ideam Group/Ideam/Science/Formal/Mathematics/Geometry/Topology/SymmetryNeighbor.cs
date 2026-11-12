using System;

using Ideam.Science.Formal.Graphs;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    [Serializable]
    public struct SymmetryNeighbor : IPort
    {
        public PortDirection direction;
        public PortDirection Direction { get { return direction; } set { direction = value; } }
        
        public int MaximumConnections { get; set; }
        
        public int neighborID;

        public static implicit operator int(SymmetryNeighbor cn)
        {
            return cn.neighborID;
        }

        public static implicit operator SymmetryNeighbor(int n)
        {
            return new SymmetryNeighbor() { neighborID = n, direction = PortDirection.Free };
        }

        public Type AcceptedType() { return typeof(ISymmetry);  }

       

    }

}
