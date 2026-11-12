using System;

using Ideam.Science.Formal.Graphs;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    [Serializable]
    public class IsoSymmetry : ISymmetry, INode
    {

        //[SerializeField]
        public int generation;
        //[SerializeField]
        public Float2 position, scale;
        //[SerializeField]
        public float angle;
        //[SerializeField]
        public bool isOnBorder;

        public int Generation { get { return generation; } set { generation = value; } }
        public Float2 Position { get { return position; } set { position = value; } }
        public Float2 Scale { get { return scale; } set { scale = value; } }
        public float Angle { get { return angle; } set { angle = value; } }

        public bool IsOnBorder { get { return isOnBorder; } set { isOnBorder = value; } }

        //[SerializeField]
        public int[] vertices = new int[0], edges = new int[0], tris = new int[0];

        public SymmetryNeighbor[] neighbors = new SymmetryNeighbor[0];

        public int[] Vertices { get { return vertices; } set { vertices = value; } }
        public int[] Edges { get { return edges; } set { edges = value; } }

        public int[] Triangles { get { return tris; } set { tris = value; } }

        public int[] Neighbors 
        {
            get 
            {
                int[] n = new int[neighbors.Length];
                for(int nn = n.Length-1; nn>=0; nn--)
                {
                    n[nn] = neighbors[nn];
                }
                return n; 
            } 
            set 
            {
                int diff = value.Length - neighbors.Length;
                if(diff > 0)
                {
                    SymmetryNeighbor[] newNeighbors = new SymmetryNeighbor[value.Length];
                    for(int i = 0; i < neighbors.Length; i++)
                    {
                        newNeighbors[i] = neighbors[i];
                        newNeighbors[i].neighborID = value[i];
                    }
                    for(int i = neighbors.Length; i < newNeighbors.Length; i++)
                    {
                        newNeighbors[i] = new SymmetryNeighbor()
                        {
                            neighborID = value[i],
                            direction = PortDirection.Free
                        };
                    }
                }
                else
                {
                    for(int i = 0; i < value.Length; i++)
                    {
                        neighbors[i].neighborID = value[i];
                    }
                }
            } 
        }

        public IsoSymmetry()
        {
            Scale = Float2.one;
            Angle = 0;
            Position = Float2.zero;
            Neighbors = new int[0];
            IsOnBorder = true;
            Edges = new int[0];
            Vertices = new int[0];
            Triangles = new int[0];
        }

        public IsoSymmetry(ISymmetry symmetry)
        {
            scale = symmetry.Scale;
            angle = symmetry.Angle;
            position = symmetry.Position;
            Neighbors = symmetry.Neighbors;
            IsOnBorder = symmetry.IsOnBorder;
            Edges = symmetry.Edges;
            Vertices = symmetry.Vertices;
            Triangles = symmetry.Triangles;
        }

        public void SetPosition(Float2 newPos)
        {
            Position = newPos;
        }

        public static IsoSymmetry[] ConvertSymmetries<T>(T[] symsToConvert) where T:ISymmetry
        {
            IsoSymmetry[] newSyms = new IsoSymmetry[symsToConvert.Length];
            for (int i = symsToConvert.Length - 1; i >= 0; i--)
            {
                newSyms[i] = new IsoSymmetry(symsToConvert[i]);
            }
            return newSyms;
        }

        public static IsoSymmetry[] ConvertSymmetries(Symmetry3[] symsToConvert)
        {
            IsoSymmetry[] newSyms = new IsoSymmetry[symsToConvert.Length];
            for (int i = symsToConvert.Length - 1; i >= 0; i--)
            {
                newSyms[i] = new IsoSymmetry(symsToConvert[i]);
            }
            return newSyms;
        }

        public static IsoSymmetry[] ConvertSymmetries(Symmetry4[] symsToConvert)
        {
            IsoSymmetry[] newSyms = new IsoSymmetry[symsToConvert.Length];
            for (int i = symsToConvert.Length - 1; i >= 0; i--)
            {
                newSyms[i] = new IsoSymmetry(symsToConvert[i]);
            }
            return newSyms;
        }

        public static IsoSymmetry[] ConvertSymmetries(Symmetry6[] symsToConvert)
        {
            IsoSymmetry[] newSyms = new IsoSymmetry[symsToConvert.Length];
            for (int i = symsToConvert.Length - 1; i >= 0; i--)
            {
                newSyms[i] = new IsoSymmetry(symsToConvert[i]);
            }
            return newSyms;
        }

        public T[] GetPorts<T>() where T : IPort
        {
            if (typeof(T).IsAssignableFrom(typeof(SymmetryNeighbor)))
            {
                T[] nt = new T[neighbors.Length];
                for(int i = 0; i < neighbors.Length; i++)
                {
                    
                    SymmetryNeighbor n = new SymmetryNeighbor() { neighborID = neighbors[i], direction = PortDirection.Free };
                    if(n is T t) { nt[i] = t; }
                }
                return nt;
            }
            return new T[0];
        }
    }


}
