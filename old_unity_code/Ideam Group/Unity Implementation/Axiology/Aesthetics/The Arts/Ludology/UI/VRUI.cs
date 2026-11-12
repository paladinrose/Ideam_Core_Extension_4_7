using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEngine.Pool;
using UnityEngine.InputSystem;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;
using Ideam.Unity.UI.WorldSpace;

using GamePhase = Ideam.Axiology.Aesthetics.The_Arts.Ludology.GamePhase;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{
    public class VRUI : MonoBehaviour
    { 
        public UnityGame game;

        public UnityGameAgent vrAgent;

        public WorldSpaceUIDocument worldSpaceUIDocument;
        public WorldSpaceUserInterface leftMenu, rightMenu, worldSpaceRuntimeExplorer;
        WorldSpaceUIElement[] runtimeExplorerElements = new WorldSpaceUIElement[0];

        public InputActionReference leftMenuToggle, rightMenuToggle, exploreObjectButton, uiSelect, uiCancel;
        public bool pauseOnLeftMenu, pauseOnRightMenu;
        
        public Transform agentContent, gamePieceContent;

        public WorldSpaceAgentExplorer agentExplorerTemplate;
        public int maxExploredAgents = 3;

        public WorldSpaceGamePieceExplorer gamePieceExplorerTemplate;
        public int maxExploredPieces = 5;

        public RuntimeExplorer runtimeExplorer;
        public GameObject runtimeExplorerTarget;
        
        public WorldSpaceUIElement pointingAtUI;
        public GameObject uiReticle;

        [HideInInspector]
        public Vector2 uiPointerPos;
        [HideInInspector]
        public Vector3 worldPointerPos;

        bool leftIsOpen, rightIsOpen, explorerIsOpen;

        ObjectPool<WorldSpaceAgentExplorer> agentExplorerPool;
        List<WorldSpaceAgentExplorer> exploredAgents;

        ObjectPool<WorldSpaceGamePieceExplorer> gamePieceExplorerPool;
        List<WorldSpaceGamePieceExplorer> exploredGamePieces;



        private void Awake()
        {
            exploredAgents = new List<WorldSpaceAgentExplorer>();
            agentExplorerPool = new ObjectPool<WorldSpaceAgentExplorer>(
                CreateAgentExplorer, 
                GetAgentExplorer, 
                ReleaseAgentExplorer, 
                DestroyAgentExplorer, 
                true, maxExploredAgents, maxExploredAgents
                );

            exploredGamePieces = new List<WorldSpaceGamePieceExplorer>();
            gamePieceExplorerPool = new ObjectPool<WorldSpaceGamePieceExplorer>(
                CreateGamePieceExplorer,
                GetGamePieceExplorer,
                ReleaseGamePieceExplorer,
                DestroyGamePieceExplorer,
                true, maxExploredPieces, maxExploredPieces
                );

            if (leftMenuToggle)
            {
                leftMenuToggle.action.Enable();
                leftMenuToggle.action.canceled +=ToggleLeftMenu;
            }

            if (rightMenuToggle)
            {
                rightMenuToggle.action.Enable();
                rightMenuToggle.action.canceled += ToggleRightMenu;
            }

            if(exploreObjectButton &&  runtimeExplorer)
            {
                exploreObjectButton.action.Enable();
                exploreObjectButton.action.canceled += OpenRuntimeExplorer;
            }

            if (uiSelect)
            {
                uiSelect.action.Enable();
                uiSelect.action.started += SelectUIStart;
                uiSelect.action.canceled += SelectUIEnd;
            }
            if (uiCancel)
            {
                uiCancel.action.Enable();
                uiCancel.action.started += CancelUIStart;
                uiCancel.action.canceled += CancelUIEnd;
            }
        }
        
        private void SelectUIStart(InputAction.CallbackContext obj)
        {
            if (pointingAtUI) { pointingAtUI.UVPointerDown(uiPointerPos); }
        }

        private void SelectUIEnd(InputAction.CallbackContext obj)
        {
            if (pointingAtUI) { pointingAtUI.UVPointerUp(uiPointerPos); }
        }

        private void CancelUIStart(InputAction.CallbackContext obj)
        {
        }

        private void CancelUIEnd(InputAction.CallbackContext obj)
        {
        }


        private void OpenRuntimeExplorer(InputAction.CallbackContext obj)
        {
            if (explorerIsOpen)
            {
                worldSpaceRuntimeExplorer.Close();
                explorerIsOpen = false;
            }
            else if (runtimeExplorerTarget && worldSpaceRuntimeExplorer)
            {
                VisualElement reElement = runtimeExplorer.Explore(runtimeExplorerTarget);
                
                reElement.style.width = 300;
                reElement.style.height = 400;
                worldSpaceRuntimeExplorer.SetRootElement(reElement);
                explorerIsOpen = true;
            }
        }
       

        private void LateUpdate()
        {
            //This might be better in update. Try in both and see.
            for(int i =0; i < exploredAgents.Count; i++)
            {
                exploredAgents[i].UpdateWorldSpace();
            }
            for(int i =0; i < exploredGamePieces.Count; i++)
            {
                exploredGamePieces[i].UpdateWorldSpace();
            }
            
            if(pointingAtUI && uiReticle)
            {
                Vector3 dir = (transform.position - worldPointerPos).normalized;
                uiReticle.transform.position = worldPointerPos + (dir*0.0001f);
                uiReticle.transform.LookAt(transform);

            } 
            else if (uiReticle)
            {
                Vector3 newPos = transform.position;
                newPos.y += 1000;
                uiReticle.transform.position = newPos;
            }
        }

        #region Agent Explorer Pool

        protected virtual void DestroyAgentExplorer(WorldSpaceAgentExplorer obj)
        {
            exploredAgents.Remove(obj);
            Destroy(obj.gameObject);
        }

        protected virtual void ReleaseAgentExplorer(WorldSpaceAgentExplorer obj)
        {
            exploredAgents.Remove(obj);
            obj.gameObject.SetActive(false);
        }

        protected virtual void GetAgentExplorer(WorldSpaceAgentExplorer obj)
        {
            obj.gameObject.SetActive(true);
            exploredAgents.Add(obj);
        }

        protected virtual WorldSpaceAgentExplorer CreateAgentExplorer()
        {
            WorldSpaceAgentExplorer ae = Instantiate(agentExplorerTemplate);
            ae.transform.SetParent(agentContent, false);
            //ae.gameObject.SetActive(true);
            return ae;

        }

        #endregion

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
            WorldSpaceGamePieceExplorer gpe = Instantiate(gamePieceExplorerTemplate);
            gpe.transform.SetParent(gamePieceContent, false);
            return gpe;
        }

        #endregion

        public void ExploreAgent(UnityGameAgent agent)
        {
            for(int i =0; i < exploredAgents.Count; i++)
            {
                if(exploredAgents[i].agent == agent) { return; }
            }
            WorldSpaceAgentExplorer agentExplorer = null;
            if (exploredAgents.Count == maxExploredAgents) { agentExplorer = exploredAgents[0]; exploredAgents.RemoveAt(0); exploredAgents.Add(agentExplorer); }
            else { agentExplorer = agentExplorerPool.Get(); }

            string nameFieldOverride = string.Empty;

            if (vrAgent)
            {
                // Use the vrAgent (and its mind) to run look skills and alter the explorer - such as using a custom name for the
                // name field, like "Mysterious Stranger" 
            }
            
            agentExplorer.SetAgent(agent, nameFieldOverride);
        }

        public void StopExploringAgent(UnityGameAgent agent)
        {
            for(int i = 0; i < exploredAgents.Count; i++)
            {
                if(exploredAgents[i].agent == agent) { agentExplorerPool.Release(exploredAgents[i]); return; }
            }
        }

        public void ExploreGamePiece(UnityGamePiece gamePiece)
        {
            for(int i = 0; i < exploredGamePieces.Count; i++)
            {
                if(exploredGamePieces[i].gamePiece == gamePiece) { return; }
            }

            WorldSpaceGamePieceExplorer pieceExplorer = null;
            if (exploredGamePieces.Count == maxExploredPieces)
            {
                pieceExplorer = exploredGamePieces[0]; exploredGamePieces.RemoveAt(0); exploredGamePieces.Add(pieceExplorer);
            }
            else { pieceExplorer = gamePieceExplorerPool.Get(); }

            string nameFieldOverride = string.Empty;

            if (vrAgent)
            {
                // Use the vrAgent (and its mind) to run look skills and alter the explorer - such as using a custom name for the
                // name field, like "Mysterious Stranger" 
            }
            
            pieceExplorer.SetGamePiece(gamePiece, nameFieldOverride);
        }

        public void StopExploringGamePiece(UnityGamePiece gamePiece)
        {
            for(int i =0; i < exploredGamePieces.Count; i++)
            {
                if(exploredGamePieces[i].gamePiece == gamePiece) { gamePieceExplorerPool.Release(exploredGamePieces[i]); return; }
            }
        }
        

        #region Left and Right Menus

        protected virtual void ToggleLeftMenu(InputAction.CallbackContext obj)
        { 
            if (!leftIsOpen) 
            { 
                if(pauseOnLeftMenu) game.PauseGame();
                leftMenu.gameObject.SetActive(true);
                leftIsOpen = true;
            }
            else 
            {
                if(pauseOnLeftMenu && !(pauseOnRightMenu && rightIsOpen)) game.ContinueGame();
                leftMenu.gameObject.SetActive(false);
                leftIsOpen = false;
            }
        }

        protected virtual void ToggleRightMenu(InputAction.CallbackContext obj)
        {
            if (!rightIsOpen)
            {
                if (pauseOnRightMenu) game.PauseGame();
                rightMenu.gameObject.SetActive(true);
                rightIsOpen = true;
            }
            else
            {
                if (pauseOnRightMenu && !(pauseOnLeftMenu && leftIsOpen)) game.ContinueGame();
                rightMenu.gameObject.SetActive(false);
                rightIsOpen = false;
            }
        }

        #endregion


        public static VRUI NewVRUI(Transform parent)
        {
            GameObject uiObject = new GameObject();
            uiObject.name = "VR UI"; 

            if (parent != null) uiObject.transform.SetParent(parent, false);

            VRUI vrui = AddVRUI(uiObject, AddVRUIContext.Full());

            return vrui;
        }
        public struct AddVRUIContext
        {
            public bool leftMenu,
                        rightMenu,
                        runtimeExplorer,
                        gamePieceExplorer,
                        gameAgentExplorer;

            public static AddVRUIContext Full()
            {
                return new AddVRUIContext() { leftMenu = true, rightMenu = true, runtimeExplorer = true, gameAgentExplorer = true, gamePieceExplorer = true };
            }
        }

        public static VRUI AddVRUI(GameObject uiObject, AddVRUIContext context)
        {
            VRUI vrui = uiObject.AddComponent<VRUI>();
            vrui.worldSpaceUIDocument = uiObject.AddComponent<WorldSpaceUIDocument>();
            
            if (context.leftMenu)
            {
                GameObject leftMenuObject = new GameObject("Lefthand Menu");
                vrui.leftMenu = leftMenuObject.AddComponent<WorldSpaceUserInterface>();
                leftMenuObject.transform.SetParent(uiObject.transform, false);
                vrui.leftMenu.worldSpaceDocument = vrui.worldSpaceUIDocument;
            }

            if (context.rightMenu)
            {
                GameObject rightMenuObject = new GameObject("Righthand Menu");
                vrui.rightMenu = rightMenuObject.AddComponent<WorldSpaceUserInterface>();
                rightMenuObject.transform.SetParent(uiObject.transform, false);
                vrui.rightMenu.worldSpaceDocument = vrui.worldSpaceUIDocument;
            }
            

            if (context.gamePieceExplorer)
            {
                GameObject pieceContent = new GameObject("GamePiece Content");
                pieceContent.transform.SetParent(uiObject.transform);
                vrui.gamePieceContent = pieceContent.transform;

                GameObject pieceExplorerTemplateObject = new GameObject("GamePiece Explorer");
                pieceExplorerTemplateObject.transform.SetParent(uiObject.transform);
                WorldSpaceGamePieceExplorer pieceExplorerTemplate = pieceExplorerTemplateObject.AddComponent<WorldSpaceGamePieceExplorer>();
                vrui.gamePieceExplorerTemplate = pieceExplorerTemplate;
            }

            if (context.gameAgentExplorer)
            {
                GameObject agentContent = new GameObject("Agent Content");
                agentContent.transform.SetParent(uiObject.transform);
                vrui.agentContent = agentContent.transform;

                GameObject agentExplorerTemplateObject = new GameObject("Agent Explorer");
                agentExplorerTemplateObject.transform.SetParent(uiObject.transform);
                WorldSpaceAgentExplorer agentExplorerTemplate = agentExplorerTemplateObject.AddComponent<WorldSpaceAgentExplorer>();
                vrui.agentExplorerTemplate = agentExplorerTemplate;
            }

            if (context.runtimeExplorer)
            {
                vrui.runtimeExplorer = uiObject.AddComponent<RuntimeExplorer>();
                GameObject runtimeMenuObject = new GameObject("Runtime Explorer");
                vrui.worldSpaceRuntimeExplorer = runtimeMenuObject.AddComponent<WorldSpaceUserInterface>();

                if (vrui.leftMenu) { runtimeMenuObject.transform.SetParent(vrui.leftMenu.transform, false); }
                else { runtimeMenuObject.transform.SetParent(uiObject.transform, false); }
                
                vrui.worldSpaceRuntimeExplorer.worldSpaceDocument = vrui.worldSpaceUIDocument;
            }

            return vrui;
        }
    }
}