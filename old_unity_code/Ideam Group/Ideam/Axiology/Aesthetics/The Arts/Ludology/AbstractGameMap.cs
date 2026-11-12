using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Graphs;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;

using Ideam.Science.Applied.Cartography.Maps;
using Ideam.Science.Applied.Cartography;

namespace Ideam.Axiology.Aesthetics.The_Arts.Ludology
{
    public abstract class AbstractGameMap<TGrid, TSymmetry> : IGameBoard, IGraphedMap, IGridBasedMap<TGrid, TSymmetry>
        where TGrid : SymmetryGroup<TGrid, TSymmetry>
        where TSymmetry:ISymmetry
    {
        #region IGameBoard Implementation

        public abstract T[] GetGamePieces<T>() where T : IGamePiece;
        
        #endregion

        #region IMap Implementation

        public abstract T[] GetFeatures<T>() where T : IFeature;

        #endregion

        #region IGraph Implementation

        #region Nodes

        public virtual int NodesCount { get; }

        public abstract T GetNode<T>(int id) where T : INode;
        public abstract T[] GetNodes<T>() where T : INode;
        public abstract int AddNode<T>(T node) where T : INode;
        public abstract void RemoveNode(int localID);

        #endregion

        #region Edges

        public virtual int EdgesCount { get; }

        public abstract T GetEdge<T>(int id) where T : IEdge;
        public abstract T[] GetEdges<T>() where T : IEdge;
        public abstract int AddEdge<T>(T edge) where T : IEdge;
        public abstract void RemoveEdge(int localID);

        #endregion

        public virtual int[] GraphSearch(int startCell, int endCell)
        {
            return grid.GraphSearch(startCell, endCell);   
        }

        #endregion

        #region ISymmetryGroup Implementation

        public TGrid grid;
        public virtual TGrid Grid { get { return grid; } }

        public virtual int[] GetGridFootprint<T>(T feature) where T : IFeature
        {
            Float2[] featureFootprint = feature.CoordinatesFootprint<Float2>();
            List<int> gridCoords = new List<int>();
            for (int i = 0; i < featureFootprint.Length; i++)
            {
                int gridID = -1;
                if (grid.ContainsPoint(featureFootprint[i], ref gridID))
                {
                    if (!gridCoords.Contains(gridID)) gridCoords.Add(gridID);
                }
            }
            return gridCoords.ToArray();
        }

        #endregion
    }
}