using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Graphs
{
    /// <summary>
    /// Defines the basic structure for a graph of nodes, each with one or more ports that connect to other nodes via edges.
    /// </summary>
    public interface IGraph
    {
        #region Nodes

        T GetNode<T>(int id) where T : INode;
        T[] GetNodes<T>() where T : INode;
        
        int NodesCount { get; }
        int AddNode<T>(T node) where T:INode;
        void RemoveNode(int localID);

        #endregion

        #region Edges
        T GetEdge<T>(int id) where T : IEdge;
        T[] GetEdges<T>() where T : IEdge;
        int EdgesCount { get; }
        int AddEdge<T>(T edge) where T : IEdge;
        void RemoveEdge(int localID);

        #endregion 

        int[] GraphSearch(int startCell, int endCell);
    }
}
