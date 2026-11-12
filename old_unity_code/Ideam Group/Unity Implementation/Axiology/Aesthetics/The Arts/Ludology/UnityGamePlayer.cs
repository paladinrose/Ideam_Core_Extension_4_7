using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Axiology.Aesthetics.The_Arts.Ludology;
using Ideam.Metaphysics.Ontology.Action;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class UnityGamePlayer :MonoBehaviour, IGamePlayer
    {
        public virtual event Action<IAgent> OnTakeControlOfAgent, OnReleaseAgent;
        
        public UnityGameAgent token;

        public GhostAgent ghostAgent;

        protected UnityGameAgent toPossess;
        
        public float possessionTimer, endPossessionTimer;

        protected Coroutine takingAgentControlRoutine = null, agentReleaseRoutine = null;

        #region MonoBehaviour Methods

        protected virtual void Awake()
        {
            if (ghostAgent != null) SetGhostAgent(ghostAgent);

        }
        protected virtual void OnEnable() { }
        protected virtual void OnDisable() { }

        protected virtual void OnDestroy() { }


        
        #endregion

        public virtual void SetGhostAgent(GhostAgent newGhostAgent)
        {
            RemoveGhostAgent();

            ghostAgent = newGhostAgent;
        }
        public virtual void RemoveGhostAgent()
        {
            if (ghostAgent)
            {

            }
        }

        public virtual T[] GetControlledAgents<T>() where T : IAgent
        {
            List<T> tAgents = new List<T>();
            if (token && token is T tToken) { tAgents.Add(tToken); }

            return tAgents.ToArray();
        }

        public virtual T[] GetPlayerControlledPieces<T>() where T : IGamePiece
        {
            UnityGamePiece[] tokenPieces = token.gamePieces;
            List<T> tokenTs = new List<T>();
            for(int i =0; i < tokenPieces.Length; i++)
            {
                if(tokenPieces[i] is T tokenT) { tokenTs.Add(tokenT); }
            }
            return tokenTs.ToArray();
        }


        public virtual int TakeControlOfAgent<T>(T agent) where T : IAgent
        {
            if(agent is UnityGameAgent gameAgent)
            {
                toPossess = gameAgent;
                return TakeControlOfAgent(toPossess);
            }
            return -1;
        }

        public virtual int TakeControlOfAgent(UnityGameAgent toPossess)
        {
            if(agentReleaseRoutine != null)
            {

            }

            if(takingAgentControlRoutine != null)
            {

            }

            this.toPossess = toPossess;
            
            takingAgentControlRoutine = StartCoroutine(TakingAgentControl());

            return 0;
        }

        protected virtual IEnumerator TakingAgentControl()
        {
            token = toPossess;
            
            yield break;
        }

        protected virtual void ControlTakenOverAgent()
        {
            ghostAgent.SetPossessedAgent(token);
            toPossess = null;
            takingAgentControlRoutine = null;
            OnTakeControlOfAgent?.Invoke(token);
        }

        public virtual bool ReleaseAgent(int agentID)
        {
            if(takingAgentControlRoutine != null)
            {

            }
            if(agentReleaseRoutine != null)
            {

            }

            agentReleaseRoutine = StartCoroutine(EndingAgentControl());
            return true;
        }

        protected virtual IEnumerator EndingAgentControl()
        {
            
            yield break;
        }

        protected virtual void AgentReleased()
        {
            agentReleaseRoutine = null;
            OnReleaseAgent?.Invoke(token);
            token = null;
        }
    }
}