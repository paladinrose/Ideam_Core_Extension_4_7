namespace Ideam.Science.Formal.Graphs
{
    /// <summary>
    /// Defines the structure for an edge, connecting two nodes on a graph.
    /// </summary>
    public interface IEdge
    {
        int NodeA { get; set; }
        int PortA { get; set; }
        int NodeB { get; set; }
        int PortB { get; set; }
    }
}
