namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    public interface ISymmetry
    {

        int Generation { get; set; }
        Float2 Position { get; set; }
        Float2 Scale { get; set; }
        float Angle { get; set; }
        bool IsOnBorder { get; set; }
        int[] Vertices { get; set; }
        int[] Edges { get; set; }
        int[] Triangles { get; }
        int[] Neighbors { get; set; }

    }

}
