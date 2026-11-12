using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Science.Formal.Graphs;
using Ideam.Science.Formal.Mathematics;

using Ideam.Unity.Metaphysics.Mind;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class UnityAIController : FirstPersonController
    {
        public UnityMind controlledMind;
        public AgentAIGraph AIGraph;
        
        #region Monobehaviour Methods

        private void Awake()
        {
            if (controlledMind)
            {
                controlledMind.OnStartCognitiveUpdate += StartCognitiveUpdate;
                controlledMind.OnCompleteCognitiveUpdate += CompleteCognitiveUpdate;
            }
        }

        private void Update()
        {
            if (controlledMind) { controlledMind.CognitiveUpdate(); }
        }

        #endregion

        protected override void SetupController()
        {
            base.SetupController();
            
        }

        #region Cognitive Updates

        private void StartCognitiveUpdate(UnityMind obj)
        {
        }

        private void CompleteCognitiveUpdate(UnityMind obj)
        {
        }

        #endregion

        public static UnityAIController NewUnityAIController(Transform parent)
        {
            string baseName = "New AI Controller";

            GameObject controllerObject = new GameObject(baseName);
            if (parent != null)
            {
                controllerObject.transform.SetParent(parent);
                controllerObject.transform.localEulerAngles = controllerObject.transform.localPosition = Vector3.zero;

            }
            UnityAIController controller = AddUnityAIController(controllerObject);

            return controller;
        }

        public static UnityAIController AddUnityAIController(GameObject controllerObject, Rigidbody rigidbody = null)
        {
            UnityAIController controller = controllerObject.GetComponent<UnityAIController>();
            if (controller == null)
            { 
                controller = controllerObject.AddComponent<UnityAIController>();

                AddDefaultControls(controllerObject, controller, rigidbody);
                controller.walkSpeed = 1f;
                controller.runSpeed = 1.5f;
                controller.crouchSpeed = 0.5f;

                controller.StandingHeight = 1.5f;
                controller.CrouchingHeight = 1.15f;

                AgentAIGraph defaultAI = new AgentAIGraph();

                AgentAINode navigateState = new AgentAINode() { name = "Navigate" };
            }

            return controller;
        }
    }

    [Serializable]
    public class AgentAIGraph : IGraph
    {
        public AgentAINode[] states = new AgentAINode[0];
        public AgentAIEdge[] edges = new AgentAIEdge[0];

        public int NodesCount { get { return states.Length; } }
        public int EdgesCount { get { return edges.Length; } }

        public int AddEdge<T>(T edge) where T : IEdge
        {
            if(edge is AgentAIEdge aiEdge)
            {
                int id = edges.Length;
                System.Array.Resize(ref edges, id + 1);
                edges[id] = aiEdge;
                return id;
            }

            return -1;
        }

        public T GetEdge<T>(int id) where T : IEdge
        {
            throw new NotImplementedException();
        }
        public T[] GetEdges<T>() where T : IEdge
        {
            throw new NotImplementedException();
        }
        public void RemoveEdge(int localID)
        {
            throw new NotImplementedException();
        }

        public int AddNode<T>(T node) where T : INode
        {
            throw new NotImplementedException();
        }

        public T GetNode<T>(int id) where T : INode
        {
            throw new NotImplementedException();
        }

        public T[] GetNodes<T>() where T : INode
        {
            throw new NotImplementedException();
        }

        public void RemoveNode(int localID)
        {
            throw new NotImplementedException();
        }

        public int[] GraphSearch(int startNode, int endNode)
        {
            throw new NotImplementedException();
        }


    }

    [Serializable]
    public class AgentAINode : INode
    {
        public string name;
        public AgentAIPort[] stateChanges;

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

    [Serializable]
    public class AgentAIEdge : IEdge
    {
        public int NodeA { get; set; }
        public int PortA { get; set; }
        public int NodeB { get; set; }
        public int PortB { get; set; }
    }

    [Serializable]
    public class AgentAIPort : IPort
    {
        public int MaximumConnections { get; set; }
        public PortDirection Direction { get; set; }

        public Type AcceptedType()
        {
            throw new NotImplementedException();
        }
    }
}
