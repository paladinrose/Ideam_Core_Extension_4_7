using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.Semantics;
using Ideam.Unity.UI;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    [UserInterface(typeof(UnityGamePiece))]
    public class UnityGamePieceUI : UnityGamePieceUI<UnityGamePiece> { }
    
    public class UnityGamePieceUI<T> : UserInterface<T>
        where T : UnityGamePiece
    {
        public override event Action OnUIUpdate;

        protected TextField gamePieceLabel;
        protected Foldout gamePieceFoldout, gameInfoFoldout, semanticsFoldout;
        protected ObjectSelectorField gameSelector, gameAgentSelector, gameBoardSelector,
                                      semanticManagerSelector, languageAssetSelector;

        protected UnityGamePiecePropertiesDrop propertiesElement;
        
        protected UnityGamePieceResourcesDrop resourcesElement;
       
        protected ObjectSelectorList subPiecesList;

        public bool AllowAddingProperties
        {
            get { if (propertiesElement == null) { return false; } return propertiesElement.AllowAddingSemantics; }
            set
            {
                if (propertiesElement != null)
                {
                    
                    if (value) { propertiesElement.OnAdd += evt => { OnUIUpdate?.Invoke(); }; }
                    else { propertiesElement.OnAdd -= evt => { OnUIUpdate?.Invoke(); }; }

                    propertiesElement.AllowAddingSemantics = value;
                }
            }
        }

        public bool AllowRemovingProperties
        {
            get { if (propertiesElement == null) { return false; } return propertiesElement.AllowRemovingSemantics; }
            set 
            {
                if (propertiesElement != null) 
                {
                    
                    if (value) { propertiesElement.OnRemove += evt => { OnUIUpdate?.Invoke(); }; }
                    else { propertiesElement.OnRemove -= evt => { OnUIUpdate?.Invoke(); }; }

                    propertiesElement.AllowRemovingSemantics = value;
                }
            }
        }

        public bool AllowAddingResources
        {
            get { if (resourcesElement == null) { return false; } return resourcesElement.AllowAddingSemantics; }
            set 
            {
                if (resourcesElement != null) 
                {
                    if (value) { resourcesElement.OnAdd += evt => { OnUIUpdate?.Invoke(); }; }
                    else { resourcesElement.OnAdd -= evt => { OnUIUpdate?.Invoke(); }; }

                    resourcesElement.AllowAddingSemantics = value; 
                }
            }
        }
        public bool AllowRemovingResources
        {
            get { if (resourcesElement == null) { return false; } return resourcesElement.AllowRemovingSemantics; }
            set 
            { 
                if (resourcesElement != null) 
                {
                    if (value) { resourcesElement.OnRemove += evt => { OnUIUpdate?.Invoke(); }; }
                    else { resourcesElement.OnRemove -= evt => { OnUIUpdate?.Invoke(); }; }
                    resourcesElement.AllowRemovingSemantics = value;
                } 
            }
        }

        protected bool showGamePieceFoldout = true;
        public bool ShowGamePieceFoldout 
        { 
            get { return showGamePieceFoldout; } 
            set 
            { 
                if(gamePieceFoldout == null) { showGameInfoFoldout = false; return; }
                if(value != showGamePieceFoldout) 
                { 
                    showGamePieceFoldout = value;
                    if (showGamePieceFoldout) { gamePieceFoldout.style.display = DisplayStyle.Flex; }
                    else { gamePieceFoldout.style.display = DisplayStyle.None; }
                }
            }
        }

        protected bool showGameInfoFoldout;
        public bool ShowGameInfoFoldout
        {
            get { return showGameInfoFoldout; }
            set
            {
                if(gameInfoFoldout == null) { showGameInfoFoldout = false; return; }

                if (value != showGameInfoFoldout)
                {
                    showGameInfoFoldout = value;
                    if (showGameInfoFoldout) { gameInfoFoldout.style.display = DisplayStyle.Flex; }
                    else { gameInfoFoldout.style.display = DisplayStyle.None; }
                }
            }
        }

        protected bool allowAddingSubPieces, allowRemovingSubPieces;
        public bool AllowAddingSubPieces
        {
            get { return allowAddingSubPieces; }
            set
            {
                if (subPiecesList == null) { allowAddingSubPieces = false; return; }
                if (value != allowAddingSubPieces)
                {
                    allowAddingSubPieces = value;
                    if (allowAddingSubPieces)
                    {
                        subPiecesList.OnAdd += AddSubPiece;
                    }
                    else
                    {
                        subPiecesList.OnAdd -= AddSubPiece;
                    }
                }
            }
        }

        public bool AllowRemovingSubPieces 
        {
            get { return allowRemovingSubPieces; }
            set
            {
                if(subPiecesList == null) { allowRemovingSubPieces = false; return; }
                if(value != allowRemovingSubPieces)
                {
                    allowRemovingSubPieces = value;
                    if (allowRemovingSubPieces)
                    {
                        subPiecesList.OnRemove += RemoveSubPiece;
                    }
                    else
                    {
                        subPiecesList.OnRemove -= RemoveSubPiece;
                    }
                }
            }
        }

        public override void SubscribeToObjectEvents()
        {
            
        }

        public override void UnsubscribeFromObjectEvents()
        {

        }

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) return false;

            gamePieceFoldout = ui.Q<Foldout>("GamePieceFoldout");
            
            gameInfoFoldout = ui.Q<Foldout>("GameInfoFoldout");

            semanticsFoldout = ui.Q<Foldout>("SemanticsFoldout");
            
            semanticManagerSelector = ui.Q<ObjectSelectorField>("SemanticManagerSelector");
            if (semanticManagerSelector != null) 
                semanticManagerSelector.Initialize(OnSelectSemanticManager, sourceObject.semanticManager,null, ObjectSelectorField.ObjectSearchMode.Resources);
            
            languageAssetSelector = ui.Q<ObjectSelectorField>("LanguageAssetSelector");
            if (languageAssetSelector != null) 
                languageAssetSelector.Initialize(OnSelectLanguageAsset, sourceObject.languageAsset, null, ObjectSelectorField.ObjectSearchMode.Resources);
            
            gameSelector = ui.Q<ObjectSelectorField>("GameSelector");
            if (gameSelector != null)
                gameSelector.Initialize(OnSelectGame, sourceObject.game, null, ObjectSelectorField.ObjectSearchMode.Resources);
            
            gameAgentSelector = ui.Q<ObjectSelectorField>("GameAgentSelector");
            if (gameAgentSelector != null)
                gameAgentSelector.Initialize(OnSelectGameAgent, sourceObject.gameAgent, sourceObject.gameObject);
            
            gameBoardSelector = ui.Q<ObjectSelectorField>("GameBoardSelector");
            if (gameBoardSelector != null)
                gameBoardSelector.Initialize(OnSelectGameBoard, sourceObject.gameBoard);
            


            propertiesElement = ui.Q<UnityGamePiecePropertiesDrop>("PropertiesDrop");
            if (propertiesElement != null)
            { 
                propertiesElement.GamePiece = sourceObject; 
                //propertiesElement.OnAdd += evt => { OnUIUpdate?.Invoke(); };
                //propertiesElement.OnRemove += evt => { OnUIUpdate?.Invoke(); };
                propertiesElement.OnSetPropertyValue += (id, evt) => { OnUIUpdate?.Invoke(); };
            }
            
            resourcesElement = ui.Q<UnityGamePieceResourcesDrop>("ResourcesDrop");
            if (resourcesElement != null)
            { 
                resourcesElement.GamePiece = sourceObject; 
                //resourcesElement.OnAdd += evt => { OnUIUpdate?.Invoke(); };
                //resourcesElement.OnRemove += evt => { OnUIUpdate?.Invoke(); };
                resourcesElement.OnSetResourceValue += (id, evt) => { OnUIUpdate?.Invoke(); };
                resourcesElement.OnSetResourceMaxValue += (id, evt) => { OnUIUpdate?.Invoke(); };
            }
            
            subPiecesList = ui.Q<ObjectSelectorList>("SubPiecesList");
            if (subPiecesList != null)
            {
                subPiecesList.BuildList<UnityGamePiece>(SubPieceSetInSlot, sourceObject.subPieces);
                subPiecesList.OnAdd += evt => { OnUIUpdate?.Invoke(); };
            }
            return true;
        }


        #region Game Info

        protected virtual void OnSelectGame(UnityGame selection)
        {
            if (sourceObject.game == selection) { return; }

            sourceObject.game = selection;
            OnUIUpdate?.Invoke();
        }

        protected virtual void OnSelectGameAgent(UnityGameAgent selection)
        {
            if (sourceObject.gameAgent == selection) { return; }

            sourceObject.gameAgent = selection;
            OnUIUpdate?.Invoke();
        }

        protected virtual void OnSelectGameBoard(UnityGameBoard selection)
        {
            if (sourceObject.gameBoard == selection) { return; }

            sourceObject.gameBoard = selection;
            OnUIUpdate?.Invoke();
        }

        #endregion

        #region Sub-Pieces

        private void SubPieceSetInSlot(int listID, UnityGamePiece selection)
        {
            if (sourceObject.subPieces[listID] == selection) { return; }
            sourceObject.subPieces[listID] = selection;
            OnUIUpdate?.Invoke();
        }

        protected virtual void AddSubPiece(int obj)
        {
            
            OnUIUpdate?.Invoke();
        }

        protected virtual void RemoveSubPiece(int obj)
        {

            OnUIUpdate?.Invoke();
        }

        #endregion

        #region Semantics

        protected virtual void OnSelectSemanticManager(UnitySemanticManager selection)
        {
            if (sourceObject.semanticManager == selection) { return; }

            sourceObject.semanticManager = selection;
            OnUIUpdate?.Invoke();
        }

        protected virtual void OnSelectLanguageAsset(UnityLanguageAsset selection)
        {
            if (sourceObject.languageAsset == selection) { return; }

            sourceObject.languageAsset = selection;
            OnUIUpdate?.Invoke();
        }

        

        #endregion

    }
}