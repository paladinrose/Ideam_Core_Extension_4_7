using System;

using Ideam.Science.Formal.Graphs;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    [Serializable]
    public struct Symmetry4 : ISymmetry, INode
    {
        public Float2 position, scale;
        public float angle;

        public bool isOnBorder;
        public int generation;

        public Float2 Position { get { return position; } set { position = value; } }
        public Float2 Scale { get { return scale; } set { scale = value; } }
        public float Angle { get { return angle; } set { angle = value; } }

        public bool IsOnBorder { get { return isOnBorder; } set { isOnBorder = value; } }
        public int Generation { get { return generation; } set { generation = value; } }

        public int vert1, vert2, vert3, vert4,
                   edge1, edge2, edge3, edge4;
        public SymmetryNeighbor neighbor1, neighbor2, neighbor3, neighbor4;

        public int[] Vertices
        {
            get { return new int[] { vert1, vert2, vert3, vert4 }; }
            set
            {
                if (value.Length > 0)
                {
                    switch (value.Length)
                    {
                        case 1:
                            vert1 = value[0];
                            break;
                        case 2:
                            vert1 = value[0];
                            vert2 = value[1];
                            break;
                        case 3:
                            vert1 = value[0];
                            vert2 = value[1];
                            vert3 = value[2];
                            break;
                        default:
                            vert1 = value[0];
                            vert2 = value[1];
                            vert3 = value[2];
                            vert4 = value[3];
                            break;
                    }
                }
            }
        }

        public int[] Edges
        {
            get { return new int[] { edge1, edge2, edge3, edge4 }; }
            set
            {
                if (value.Length > 0)
                {
                    switch (value.Length)
                    {
                        case 1:
                            edge1 = value[0];
                            break;
                        case 2:
                            edge1 = value[0];
                            edge2 = value[1];
                            break;
                        case 3:
                            edge1 = value[0];
                            edge2 = value[1];
                            edge3 = value[2];
                            break;
                        default:
                            edge1 = value[0];
                            edge2 = value[1];
                            edge3 = value[2];
                            edge4 = value[3];
                            break;
                    }
                }
            }
        }

        public int[] Neighbors
        {
            get { return new int[] { neighbor1, neighbor2, neighbor3, neighbor4 }; }
            set
            {
                if (value.Length > 0)
                {
                    switch (value.Length)
                    {
                        case 1:
                            neighbor1.neighborID = value[0];
                            break;
                        case 2:
                            neighbor1.neighborID = value[0];
                            neighbor2.neighborID = value[1];
                            break;
                        case 3:
                            neighbor1.neighborID = value[0];
                            neighbor2.neighborID = value[1];
                            neighbor3.neighborID = value[2];
                            break;
                        default:
                            neighbor1.neighborID = value[0];
                            neighbor2.neighborID = value[1];
                            neighbor3.neighborID = value[2];
                            neighbor4.neighborID = value[3];
                            break;
                    }
                }
            }
        }

        public int[] Triangles
        {
            get
            {
                int[] tris = new int[6];
                tris[1] = vert2;
                tris[4] = vert4;
                if (Scale.x * Scale.y >= 0)
                {
                    tris[0] = vert1;
                    tris[2] = vert3;

                    tris[3] = vert3;
                    tris[5] = vert1;
                }
                else
                {
                    tris[0] = vert3;
                    tris[2] = vert1;

                    tris[3] = vert1;
                    tris[5] = vert3;
                }
                return tris;
            }

        }


        public Symmetry4(Symmetry4 baseSymmetry)
        {
            vert1 = baseSymmetry.vert1;
            vert2 = baseSymmetry.vert2;
            vert3 = baseSymmetry.vert3;
            vert4 = baseSymmetry.vert4;

            edge1 = baseSymmetry.edge1;
            edge2 = baseSymmetry.edge2;
            edge3 = baseSymmetry.edge3;
            edge4 = baseSymmetry.edge4;


            neighbor1 = baseSymmetry.neighbor1;
            neighbor2 = baseSymmetry.neighbor2;
            neighbor3 = baseSymmetry.neighbor3;
            neighbor4 = baseSymmetry.neighbor4;

            position = baseSymmetry.position;
            scale = baseSymmetry.scale;
            angle = baseSymmetry.angle;

            isOnBorder = baseSymmetry.IsOnBorder;
            generation = baseSymmetry.Generation + 1;
        }

        public Symmetry4(Float2 pos, Float2 scl, float ang)
        {
            vert1 = vert2 = vert3 = vert4 = edge1 = edge2 = edge3 = edge4 =
            neighbor1 = neighbor2 = neighbor3 = neighbor4 = -1;

            position = pos; scale = scl; angle = ang;
            isOnBorder = true;
            generation = 0;
        }
        public void SetPosition(Float2 newPos)
        {
            Position = newPos;
        }
        public T[] GetPorts<T>() where T : IPort
        {
            if (typeof(T).IsAssignableFrom(typeof(SymmetryNeighbor)))
            {
                SymmetryNeighbor[] n = new SymmetryNeighbor[4];
                n[0] = new SymmetryNeighbor() { neighborID = neighbor1, direction = PortDirection.Free };
                n[1] = new SymmetryNeighbor() { neighborID = neighbor2, direction = PortDirection.Free };
                n[2] = new SymmetryNeighbor() { neighborID = neighbor3, direction = PortDirection.Free };
                n[3] = new SymmetryNeighbor() { neighborID = neighbor4, direction = PortDirection.Free };

                T[] nt = new T[n.Length];

                if (n[0] is T t0) { nt[0] = t0; }
                if (n[1] is T t1) { nt[1] = t1; }
                if (n[2] is T t2) { nt[2] = t2; }
                if (n[3] is T t3) { nt[3] = t3; }

                return nt;
            }
            return new T[0];
        }
    }

}
