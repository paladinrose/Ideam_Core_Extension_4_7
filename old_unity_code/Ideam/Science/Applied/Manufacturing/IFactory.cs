using System.Collections.Generic;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Metaphysics.Ontology.Identity;
using Ideam.Metaphysics.Ontology.Action;

using Ideam.Science.Formal.Graphs;
using Ideam.Science.Formal.Mathematics;
using Ideam.Axiology.Valuation;
using System;

//using AbstractNode = Ideam.Science.Formal.Graphs.AbstractNode;
namespace Ideam.Science.Applied.Manufacturing
{
    public interface IFactory
    {
        T GetPart<T>(int id) where T : IThing;
        T[] GetParts<T>() where T : IThing;

        T GetProcess<T>(int id) where T : IProcess;
        T[] GetProcesses<T>() where T : IProcess;

        T GetProduct<T>(int id) where T : IThing;
        T[] GetProducts<T>() where T : IThing;

        void Manufacture();
    }

    public class ManufacturingNode : AbstractNode
    {
        public override T[] GetPorts<T>()
        {
            return new T[0];
        }
    }
    
    public class ManufacturingProcess : ManufacturingNode, IProcess
    {
        public int ProcessLength { get; }
        public int[] ProcessActionIDs { get; }

        public T GetProcessor<T>(int id) where T : IThing
        {
            throw new System.NotImplementedException();
        }

        public T[] GetProcessors<T>() where T : IThing
        {
            throw new System.NotImplementedException();
        }

        public void Process()
        {
            throw new System.NotImplementedException();
        }

        public void ProcessComplete()
        {
            throw new System.NotImplementedException();
        }

        public void StartProcessing()
        {
            throw new System.NotImplementedException();
        }

        public void StopProcessing()
        {
            throw new System.NotImplementedException();
        }
    }

    public class ManufacturingAction : IAction
    {
        public GamePieceUseData[] sequence;
        public GamePieceUseData[] Sequence { get { return sequence; } }

        public ActionType actionType;
        public ActionType ActionType { get { return actionType; } }
    }


    public class FactoryPart : ManufacturingNode, IThing
    {
        public ManufacturingAction[] actions;
        public FactoryMaterial[] resources;

        public T[] Actions<T>() where T : IAction
        {
            List<T> acts = new List<T>();
            for(int i = 0; i < actions.Length; i++)
            {
                if(actions[i] is T tAct) { acts.Add(tAct); }
            }
            return acts.ToArray();
        }

        public T[] Properties<T>() where T : IProperty
        {
            throw new System.NotImplementedException();
        }

        public T[] Resources<T>() where T : IResource
        {
            List<T> res = new List<T>();
            for(int i =0; i < resources.Length; i++)
            {
                if(resources[i] is T tRes)
                {
                    res.Add(tRes);
                }
            }
            return res.ToArray();
        }

        public virtual bool SetReality<T>(T reality) where T : IReality
        {
            throw new System.NotImplementedException();
        }

        public virtual void TakeAction(int id)
        {

        }
        public virtual T GenerateMaterial<T>(int id) where T : FactoryMaterial { return default; }

    }

    public class FactoryMaterial : ManufacturingNode, IResource
    {
        public int Sememe()
        {
            throw new System.NotImplementedException();
        }

        public T Value<T>() where T : IValue<T>
        {
            throw new System.NotImplementedException();
        }
        
        public T MaximumValue<T>() where T : IValue<T>
        {
            throw new System.NotImplementedException();
        }

        
        public T UseResource<T>() where T : IValue<T>
        {
            throw new System.NotImplementedException();
        }
    }

    public class FactoryProduct: FactoryPart
    {

    }

    public abstract class AbstractFactoryGraph : IFactory, IGraph
    {
        
        public FactoryPart[] parts;
        public ManufacturingProcess[] processes;
        public FactoryProduct[] products;

        #region IFactory Implementation

        public virtual T GetPart<T>(int id) where T : IThing
        {
            if(id<0 || id>=parts.Length || (parts[id] is not T tPart)) { return default; }
            return tPart;
        }

        public virtual T[] GetParts<T>() where T : IThing
        {
            List<T> tParts = new List<T>();
            for(int i =0; i < parts.Length; i++)
            {
                if(parts[i] is T tPart) { tParts.Add(tPart); }
            }
            return tParts.ToArray();
        }

        public virtual T GetProcess<T>(int id) where T : IProcess
        {
            if (id < 0 || id >= processes.Length || (processes[id] is not T tProcess)) { return default; }
            return tProcess;
        }

        public virtual T[] GetProcesses<T>() where T : IProcess
        {
            List<T> tProcesses = new List<T>();
            for (int i = 0; i < processes.Length; i++)
            {
                if (processes[i] is T tProcess) { tProcesses.Add(tProcess); }
            }
            return tProcesses.ToArray();
        }

        public virtual T GetProduct<T>(int id) where T : IThing
        {
            if (id < 0 || id >= products.Length || (products[id] is not T tProduct)) { return default; }
            return tProduct;
        }

        public virtual T[] GetProducts<T>() where T : IThing
        {
            List<T> tProducts = new List<T>();
            for (int i = 0; i < products.Length; i++)
            {
                if (products[i] is T tProduct) { tProducts.Add(tProduct); }
            }
            return tProducts.ToArray();
        }

        public abstract void Manufacture();
        
        #endregion


        #region IGraph Implementation

        public abstract int NodesCount { get; }
        
        public abstract int AddNode<T>(T node) where T : INode;
        public abstract T GetNode<T>(int id) where T : INode;
        public abstract T[] GetNodes<T>() where T : INode;
        public abstract void RemoveNode(int localID);

        public abstract int EdgesCount { get; }

        public abstract int AddEdge<T>(T edge) where T : IEdge;
        public abstract T GetEdge<T>(int id) where T : IEdge;
        public abstract T[] GetEdges<T>() where T : IEdge;
        public abstract void RemoveEdge(int localID);


        public abstract int[] GraphSearch(int startCell, int endCell);
        
        


        #endregion

    }

    public class FactoryNode : AbstractNode
    {
        public AbstractFactoryGraph factory;
        
        
        public override T[] GetPorts<T>()
        {
            if (factory == null) { return new T[0]; }
            List<T> ports = new List<T>();
            
            List<FactoryMaterial> matInputs = new List<FactoryMaterial>();
            for(int i = 0; i < factory.parts.Length; i++)
            {
                FactoryPart part = factory.parts[i];
                FactoryMaterial[] partMats = part.Resources<FactoryMaterial>();
                for(int j = 0; j < partMats.Length; j++)
                {
                    if (!matInputs.Contains(partMats[j])) { matInputs.Add(partMats[j]); }
                }
            }

            for(int i = 0; i < matInputs.Count; i++)
            {
                T port = default;
                port.Direction = PortDirection.In;
                port.MaximumConnections = 1;
                ports.Add(port);
            }

            for(int i = 0; i < factory.products.Length; i++)
            {
                T port = default;
                port.Direction = PortDirection.Out;
                port.MaximumConnections = 0;
                ports.Add(port);
            }

            return ports.ToArray();
        }
    }

    [Serializable]
    public struct ManufacturingPort : IPort, INamed
    {
        public int MaximumConnections { get; set; }
        public PortDirection Direction { get; set; }
        public string Name { get; set; }

        public Type acceptedType;
        public Type AcceptedType()
        {
            return acceptedType;
        }
    }
}

