using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Unity.Metaphysics.Mind;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    
    public class GhostAgent : UnityGameAgent
    {
        public UnityGameAgent possessedAgent;

        public override Transform AgentRoot
        {
            get
            {
                if (possessedAgent) { return possessedAgent.AgentRoot; } else { return base.AgentRoot; }
            }
            set
            {
                base.AgentRoot = value;
            }
        }

        public virtual void SetPossessedAgent(UnityGameAgent toPossess)
        {
            RemovePossessedAgent();

            possessedAgent = toPossess;
        }

        public virtual void RemovePossessedAgent()
        {
            if (possessedAgent)
            {

            }
        }



        public virtual void MapToGamepad(FirstPersonGamepad gamepad)
        {

        }


        public static GhostAgent NewGhostAgent(Transform parent = null, Animator animator = null, Rigidbody rigidbody = null, UnityGame game = null)
        {
            GameObject agentObject = new GameObject("Game Agent");
            if (animator != null) { agentObject.name = animator.gameObject.name + " (Game Agent)"; }
            if (parent != null)
            {
                agentObject.transform.SetParent(parent, false);
                agentObject.transform.localEulerAngles = agentObject.transform.localPosition = Vector3.zero;

            }

            GhostAgent agent = AddGhostAgent(agentObject, animator, rigidbody, parent, game);

            return agent;
        }

        public static GhostAgent AddGhostAgent(GameObject agentObject, Animator animator = null, Rigidbody rigidbody = null, Transform agentRoot = null, UnityGame game = null, bool makeMindObject = false)
        {
            GhostAgent agent = agentObject.AddComponent<GhostAgent>();

            agent.agentBody = rigidbody;
            agent.agentAnimator = animator;
            agent.AgentRoot = agentRoot;
            agent.game = game;

            GameObject mindObject = null;

            if (makeMindObject)
            {
                mindObject = new GameObject("Agent Mind");
                mindObject.transform.SetParent(agentObject.transform);
                mindObject.transform.localEulerAngles = mindObject.transform.localPosition = Vector3.zero;
            }
            else { mindObject = agent.gameObject; }

            agent.SetupAsGhost(game, mindObject.transform);
            
            AddDefaultActions(agent);
            
            return agent;
        }
    }
}