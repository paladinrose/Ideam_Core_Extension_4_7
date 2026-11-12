using System;

using Ideam.Science.Formal.Graphs;

namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    [Serializable]
    public struct Symmetry6 : ISymmetry, INode
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

        public int vert1, vert2, vert3, vert4, vert5, vert6,
                   edge1, edge2, edge3, edge4, edge5, edge6;

        public SymmetryNeighbor neighbor1, neighbor2, neighbor3,
                   neighbor4, neighbor5, neighbor6;

        public int[] Vertices
        {
            get { return new int[] { vert1, vert2, vert3, vert4, vert5, vert6 }; }
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
                        case 4:
                            vert1 = value[0];
                            vert2 = value[1];
                            vert3 = value[2];
                            vert4 = value[3];
                            break;
                        case 5:
                            vert1 = value[0];
                            vert2 = value[1];
                            vert3 = value[2];
                            vert4 = value[3];
                            vert5 = value[4];
                            break;
                        default:
                            vert1 = value[0];
                            vert2 = value[1];
                            vert3 = value[2];
                            vert4 = value[3];
                            vert5 = value[4];
                            vert6 = value[5];
                            break;
                    }
                }
            }
        }

        public int[] Edges
        {
            get { return new int[] { edge1, edge2, edge3, edge4, edge5, edge6 }; }
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
                        case 4:
                            edge1 = value[0];
                            edge2 = value[1];
                            edge3 = value[2];
                            edge4 = value[3];
                            break;
                        case 5:
                            edge1 = value[0];
                            edge2 = value[1];
                            edge3 = value[2];
                            edge4 = value[3];
                            edge5 = value[4];
                            break;
                        default:
                            edge1 = value[0];
                            edge2 = value[1];
                            edge3 = value[2];
                            edge4 = value[3];
                            edge5 = value[4];
                            edge6 = value[5];
                            break;
                    }
                }
            }
        }

        public int[] Neighbors
        {
            get { return new int[] { neighbor1, neighbor2, neighbor3, neighbor4, neighbor5, neighbor6 }; }
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
                        case 4:
                            neighbor1.neighborID = value[0];
                            neighbor2.neighborID = value[1];
                            neighbor3.neighborID = value[2];
                            neighbor4.neighborID = value[3];
                            break;
                        case 5:
                            neighbor1.neighborID = value[0];
                            neighbor2.neighborID = value[1];
                            neighbor3.neighborID = value[2];
                            neighbor4.neighborID = value[3];
                            neighbor5.neighborID = value[4];
                            break;
                        default:
                            neighbor1.neighborID = value[0];
                            neighbor2.neighborID = value[1];
                            neighbor3.neighborID = value[2];
                            neighbor4.neighborID = value[3];
                            neighbor5.neighborID = value[4];
                            neighbor6.neighborID = value[5];
                            break;
                    }
                }
            }
        }

        public int[] Triangles
        {
            get
            {
                int[] tris = new int[12];
                tris[1] = vert2;
                tris[4] = vert6;
                tris[7] = vert4;
                tris[10] = vert5;

                if (Scale.x * Scale.y >= 0)
                {
                    tris[0] = vert1;
                    tris[2] = vert3;

                    tris[3] = vert3;
                    tris[5] = vert1;

                    tris[6] = vert3;
                    tris[8] = vert6;

                    tris[9] = vert4;
                    tris[11] = vert6;
                }
                else
                {
                    tris[0] = vert3;
                    tris[2] = vert1;

                    tris[3] = vert1;
                    tris[5] = vert3;

                    tris[6] = vert6;
                    tris[8] = vert3;

                    tris[9] = vert6;
                    tris[11] = vert4;
                }
                return tris;
            }

        }

        public Symmetry6(Symmetry6 baseSymmetry)
        {
            vert1 = baseSymmetry.vert1;
            vert2 = baseSymmetry.vert2;
            vert3 = baseSymmetry.vert3;
            vert4 = baseSymmetry.vert4;
            vert5 = baseSymmetry.vert5;
            vert6 = baseSymmetry.vert6;

            edge1 = baseSymmetry.edge1;
            edge2 = baseSymmetry.edge2;
            edge3 = baseSymmetry.edge3;
            edge4 = baseSymmetry.edge4;
            edge5 = baseSymmetry.edge5;
            edge6 = baseSymmetry.edge6;


            neighbor1 = baseSymmetry.neighbor1;
            neighbor2 = baseSymmetry.neighbor2;
            neighbor3 = baseSymmetry.neighbor3;
            neighbor4 = baseSymmetry.neighbor4;
            neighbor5 = baseSymmetry.neighbor5;
            neighbor6 = baseSymmetry.neighbor6;

            position = baseSymmetry.position;
            scale = baseSymmetry.scale;
            angle = baseSymmetry.angle;
            isOnBorder = baseSymmetry.IsOnBorder;
            generation = baseSymmetry.Generation + 1;
        }

        public Symmetry6(Float2 pos, Float2 scl, float ang)
        {
            vert1 = vert2 = vert3 = vert4 = vert5 = vert6 =
            edge1 = edge2 = edge3 = edge4 = edge5 = edge6 =
            neighbor1 = neighbor2 = neighbor3 = neighbor4 = neighbor5 = neighbor6 = -1;

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
                SymmetryNeighbor[] n = new SymmetryNeighbor[6];
                n[0] = new SymmetryNeighbor() { neighborID = neighbor1, direction = PortDirection.Free };
                n[1] = new SymmetryNeighbor() { neighborID = neighbor2, direction = PortDirection.Free };
                n[2] = new SymmetryNeighbor() { neighborID = neighbor3, direction = PortDirection.Free };
                n[3] = new SymmetryNeighbor() { neighborID = neighbor4, direction = PortDirection.Free };
                n[4] = new SymmetryNeighbor() { neighborID = neighbor5, direction = PortDirection.Free };
                n[5] = new SymmetryNeighbor() { neighborID = neighbor6, direction = PortDirection.Free };

                T[] nt = new T[n.Length];

                if (n[0] is T t0) { nt[0] = t0; }
                if (n[1] is T t1) { nt[1] = t1; }
                if (n[2] is T t2) { nt[2] = t2; }
                if (n[3] is T t3) { nt[3] = t3; }
                if (n[4] is T t4) { nt[4] = t4; }
                if (n[5] is T t5) { nt[5] = t5; }

                return nt;
            }
            return new T[0];
        }
    }


}
