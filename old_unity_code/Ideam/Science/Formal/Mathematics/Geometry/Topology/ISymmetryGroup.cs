namespace Ideam.Science.Formal.Mathematics.Geometry.Topology
{
    public interface ISymmetryGroup<TSymmetry> where TSymmetry : ISymmetry
    {
        TSymmetry[] Symmetries { get; }
        Float2[] Vertices { get; }
        SymmetryEdge[] Edges { get; }
        int[] Triangles { get; }

        Float2[] PrimitiveVertices { get; }

        bool GroupIsDirty { get; }

        float PositionTolerance { get; set; }
        float RotationTolerance { get; set; }
        float ScaleTolerance { get; set; }

        Bounds2 Extents { get; }

        void Setup();
        void Clear();

        bool ContainsPoint(Float2 p, ref int id);
        
        Bounds2 GetSymmetryBounds(int id);
        
        bool WithinTolerance(TSymmetry a, TSymmetry b);
        
        void RemoveSymmetries(int[] ids);

        TSymmetry GetNeighbor(TSymmetry baseSymmetry, int symmetryID, int neighborID);

    }

}
