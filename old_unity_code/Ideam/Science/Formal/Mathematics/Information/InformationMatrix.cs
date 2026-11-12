using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Graphs;
using Ideam.Reason;
using System;

namespace Ideam.Science.Formal.Mathematics.Information
{
    /// <summary>
    /// The Information Matrix represents complex relationships between different kinds of information.
    /// </summary>

    public class InformationMatrix : IGraph
    {
        //Each graph traditionally encapsulates a branch of information
        public int NodesCount { get; }
        public int EdgesCount { get; }

        public int AddEdge<T>(T edge) where T : IEdge
        {
            throw new System.NotImplementedException();
        }

        public int AddNode<T>(T node) where T : INode
        {
            throw new System.NotImplementedException();
        }

        public T GetEdge<T>(int id) where T : IEdge
        {
            throw new System.NotImplementedException();
        }

        public T[] GetEdges<T>() where T : IEdge
        {
            throw new System.NotImplementedException();
        }

        public T GetNode<T>(int id) where T : INode
        {
            throw new System.NotImplementedException();
        }

        public T[] GetNodes<T>() where T : INode
        {
            throw new System.NotImplementedException();
        }

        public int[] GraphSearch(int startCell, int endCell)
        {
            throw new System.NotImplementedException();
        }

        public void RemoveEdge(int localID)
        {
            throw new System.NotImplementedException();
        }

        public void RemoveNode(int localID)
        {
            throw new System.NotImplementedException();
        }
    }

    [System.Serializable]
    public class InformationNode : INode
    {
        //Each node represents one piece of information, and manages how it connects and relates to 
        //other pieces of information.
        public Float2 position;
        public Float2 Position { get { return position; } }

        public T[] GetPorts<T>() where T : IPort
        {
            return new T[0];
        }

        public void SetPosition(Float2 newPosition)
        {
            position = newPosition;
        }
    }

    [System.Serializable]
    public class InformationPort : IPort
    {
        [SerializeField]
        protected int maxConnections;
        public int MaximumConnections { get { return maxConnections; } set { maxConnections = value; } }

        [SerializeField]
        protected PortDirection direction;
        public PortDirection Direction { get { return direction; } set { direction = value; } }

        [SerializeField]
        protected Type acceptedType;
        public Type AcceptedType()
        {
            return acceptedType;
        }

        public InformationPort(Type acceptedType)
        {
            this.acceptedType = acceptedType;
        }
    }

    [System.Serializable]
    public class Information
    {
        //Use this space to set up any info that goes between the Information and the Matrix.
        //Types of info:
        //Deductive - Facts, Statements, Modus Ponens, Modus Tollens, Syllogisms, Certain Consequences
        //Inductive - Evidence, Generalizations, Statistical Syllogisms, Analogy, Probable Conclusions
        //Abductive - Hypothesis, Observation, Experimentation, Theory
        //Beliefs - A fact, a confidence level established by induction, 
        //          and an acceptance level established by abduction.
        //Intuition - When a check is made using  logical methods (predominantly abduction, through observation)
        //            if the check fails, but is within a margin of error, it increases the odds of having 
        //            an intuitive leap, which can eventually grant a piece of information.

        public virtual InformationPort[] GetPorts()
        {
            return new InformationPort[0];
        }
        
    }

    [System.Serializable]
    public class InfoFact : Information
    {
        public Fact fact;
    }

    [System.Serializable]
    public class InfoBelief : Information
    {
        public Belief belief;
    }

    [System.Serializable]
    public class InfoEvidence : Information
    {
        public Evidence evidence;
    }

    [System.Serializable]
    public class InformationEdge : IEdge
    {
        public int NodeA { get; set; }
        public int PortA { get; set; }
        public int NodeB { get; set; }
        public int PortB { get; set; }
    }


}