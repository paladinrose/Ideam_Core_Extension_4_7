using System;

namespace Ideam.Science.Formal.Graphs
{
    /// <summary>
    /// Defines the structure for the ports that connect nodes on a graph.
    /// </summary>
    public interface IPort
    { 
        int MaximumConnections { get; set; }
        PortDirection Direction { get; set; }
        Type AcceptedType();
    }
    
}
