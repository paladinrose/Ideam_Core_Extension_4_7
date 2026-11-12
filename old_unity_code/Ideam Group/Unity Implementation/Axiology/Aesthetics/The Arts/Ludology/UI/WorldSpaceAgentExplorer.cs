using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEngine.Pool;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.UI.WorldSpace
{
    public class WorldSpaceAgentExplorer : WorldSpaceUIExplorer
    {
        public UnityGameAgent agent;
        public DialogueManager dialogueManager;
        public DialogueBox dialogueBox;
        public Transform gamePieceContent;
        public WorldSpaceGamePieceExplorer gamePieceExplorerTemplate;
        
        protected ObjectPool<WorldSpaceGamePieceExplorer> pieceExplorerPool;
        protected List<WorldSpaceGamePieceExplorer> exploredGamePieces;

        protected override void SetupUIs()
        {
            base.SetupUIs();
            BuildGamePiecePool();

            if (agent)
            {
                UnityGameAgent aG = agent;
                ClearAgent();
                SetAgent(aG);
            }
        }

        public override void UpdateWorldSpace()
        {
            base.UpdateWorldSpace();
            for (int i = 0; i < exploredGamePieces.Count; i++)
            {
                exploredGamePieces[i].UpdateWorldSpace();
            }
        }

        private void BuildGamePiecePool()
        {
            exploredGamePieces = new List<WorldSpaceGamePieceExplorer>();
            pieceExplorerPool = new ObjectPool<WorldSpaceGamePieceExplorer>(
                CreateGamePieceExplorer,
                GetGamePieceExplorer,
                ReleaseGamePieceExplorer,
                DestroyGamePieceExplorer,
                true, 5, 5
                );
        }

        public void ClearAgent()
        {

        }
        
        public void SetAgent(UnityGameAgent agent, string nameFieldOverride = "")
        {
            if(this.agent == agent) { return; }


        }



        #region GamePiece Explorer Pool

        protected virtual void DestroyGamePieceExplorer(WorldSpaceGamePieceExplorer obj)
        {
            exploredGamePieces.Remove(obj);
            Destroy(obj.gameObject);
        }

        protected virtual void ReleaseGamePieceExplorer(WorldSpaceGamePieceExplorer obj)
        {
            exploredGamePieces.Remove(obj);
            obj.gameObject.SetActive(false);
        }

        protected virtual void GetGamePieceExplorer(WorldSpaceGamePieceExplorer obj)
        {
            obj.gameObject.SetActive(true);
            exploredGamePieces.Add(obj);
        }

        protected virtual WorldSpaceGamePieceExplorer CreateGamePieceExplorer()
        {
            WorldSpaceGamePieceExplorer newGPE = Instantiate(gamePieceExplorerTemplate);
            newGPE.transform.SetParent(gamePieceContent, false);
            return newGPE;
        }

        #endregion

    }
}