using System;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Science.Formal.Graphs
{
    /// <summary>
    /// Defines the basic functionality of a node on a graph.
    /// </summary> 
    public interface INode
    {
        T[] GetPorts<T>() where T:IPort;
        Float2 Position { get; }
        void SetPosition(Float2 newPosition);
    }

    public abstract class AbstractNode : INode
    {
        public Float2 position;
        public virtual event Action<Float2> OnSetPosition;
        
        public virtual Float2 Position
        {
            get { return position; }
            protected set { position = value; OnSetPosition?.Invoke(position); }
        }

        public virtual void SetPosition(Float2 newPosition)
        {
            Position = newPosition;
        }

        public virtual void SetPositionWithoutEvent(Float2 newPosition)
        {
            position = newPosition;
        }

        public abstract T[] GetPorts<T>() where T : IPort;

    }
}
