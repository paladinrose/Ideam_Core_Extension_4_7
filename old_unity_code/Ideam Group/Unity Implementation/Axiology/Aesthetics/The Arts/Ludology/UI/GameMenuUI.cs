using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI.WorldSpace;
using Ideam.Science.Formal.Mathematics;
using System;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class GameMenuUI : MonoBehaviour
    {
        public UnityGame game;
        public VisualTreeAsset gameMenuAsset;
        public WorldSpaceUserInterface worldSpaceGameMenu;

        public Vector2Int cabinetSize, screenSize;

        protected VisualElement gameMenu = null, screenElement, cabinetElement, saveGameMenu, loadGameMenu, optionsMenu;

        protected Label gameTitle;
        protected VisualElement gameGraphic;
        protected Button startGame, 
                         saveGame, returnFromSaveMenu, saveAndClose, 
                         loadGame, returnFromLoadMenu, loadAndClose, 
                         quitGame, returnFromQuitMenu, doQuit, 
                         returnToGame, 
                         options, returnFromOptionsMenu, applyOptionsAndClose;
        protected TextField saveGameName;

        protected ScrollView saveScroll, loadScroll, optionsScroll;
        public VisualElement GameMenu { get { if (gameMenu == null) { GenerateMenu(); } return gameMenu; } }

        private void OnEnable()
        {
            if (worldSpaceGameMenu)
            {
                worldSpaceGameMenu.OnRegisterUIWithDocument += BuildGameMenu;
            }
        }

        private void BuildGameMenu()
        {
            worldSpaceGameMenu.SetRootElement(GameMenu);
        }

        protected virtual void GenerateMenu()
        {
            if (!game) { return; }

            if (gameMenuAsset)
            {
                gameMenu = gameMenuAsset.CloneTree();
                
                cabinetElement = gameMenu.Q<VisualElement>("Cabinet");
                screenElement = gameMenu.Q<VisualElement>("Screen");

                gameTitle = gameMenu.Q<Label>("GameTitle");

                gameGraphic = gameMenu.Q<VisualElement>("GameGraphic");

                startGame = gameMenu.Q<Button>("StartGame");
                saveGame = gameMenu.Q<Button>("SaveGame");
                loadGame = gameMenu.Q<Button>("LoadGame");
                quitGame = gameMenu.Q<Button>("QuitGame");
                returnToGame = gameMenu.Q<Button>("ReturnToGame");
                options = gameMenu.Q<Button>("OptionsButton");

                saveGameMenu = gameMenu.Q<VisualElement>("SaveGameMenu");
                saveGameName = gameMenu.Q<TextField>("SaveGameName");
                returnFromSaveMenu = gameMenu.Q<Button>("ReturnFromSave");
                saveAndClose = gameMenu.Q<Button>("SaveAndClose");
                saveScroll = gameMenu.Q<ScrollView>("SaveGamesScroll");

                returnFromLoadMenu = gameMenu.Q<Button>("ReturnFromLoad");
                loadAndClose = gameMenu.Q<Button>("LoadAndClose");
                loadScroll = gameMenu.Q<ScrollView>("LoadGamesScroll");

                returnFromOptionsMenu = gameMenu.Q<Button>("ReturnFromOptions");
                applyOptionsAndClose = gameMenu.Q<Button>("ApplyOptionsAndClose");
                optionsScroll = gameMenu.Q<ScrollView>("OptionsScroll");
            }
            else
            {
                gameMenu = new VisualElement() { name = "GameMenu" };
                
                cabinetElement = new VisualElement() { name = "Cabinet" };
                gameMenu.Add(cabinetElement);

                screenElement = new VisualElement() { name = "Screen" };
                gameMenu.Add(screenElement);

                gameGraphic = new VisualElement() { name = "GameGraphic" };
                gameGraphic.style.position = Position.Absolute;
                gameGraphic.style.left = 0; gameGraphic.style.right = 0;
                gameGraphic.style.top = 0; gameGraphic.style.bottom = 0;
                screenElement.Add(gameGraphic);

                gameTitle = new Label();
                screenElement.Add(gameTitle);

                VisualElement buttons = new VisualElement();
                screenElement.Add(buttons);

                startGame = new Button() { name = "StartGame" };
                buttons.Add(startGame);

                saveGame = new Button() { name = "SaveGame" };
                buttons.Add(saveGame);

                loadGame = new Button() { name = "LoadGame" };
                buttons.Add(loadGame);

                quitGame = new Button() { name = "QuitGame" };
                buttons.Add(quitGame);

                returnToGame = new Button() { name = "ReturnToGame" };
                buttons.Add(returnToGame);

                options = new Button() { name = "OptionsButton" };
                buttons.Add(options);

                saveGameMenu = new VisualElement() { name = "SaveGameMenu" };
                saveGameMenu.style.position = Position.Absolute;
                saveGameMenu.style.left = saveGameMenu.style.right = saveGameMenu.style.top = saveGameMenu.style.bottom = 0;
                screenElement.Add(saveGameMenu);

                var saveLabel = new Label("Save Game");
                saveGameMenu.Add(saveLabel);

                saveScroll = new ScrollView() { name = "SaveGamesScroll" };
                saveGameMenu.Add(saveScroll);

                saveGameName = new TextField() { name = "SaveGameName", label = "Save Game Name"};
                saveGameMenu.Add(saveGameName);

                VisualElement saveButtonElement = new VisualElement();
                saveButtonElement.style.flexDirection = FlexDirection.Row;
                saveGameMenu.Add(saveButtonElement);
                
                saveAndClose = new Button() { name = "SaveAndClose", text = "Save" };
                saveButtonElement.Add(saveAndClose);
                saveAndClose.SetEnabled(false);

                returnFromSaveMenu = new Button() {name = "ReturnFromSave", text = "Cancel"};
                saveButtonElement.Add(returnFromSaveMenu);

                saveGameMenu.style.display = DisplayStyle.None;


                loadGameMenu = new VisualElement() { name = "LoadGameMenu" };
                loadGameMenu.style.position = Position.Absolute;
                loadGameMenu.style.left = loadGameMenu.style.right = loadGameMenu.style.top = loadGameMenu.style.bottom = 0;
                screenElement.Add(loadGameMenu);

                var loadLabel = new Label("Load Game");
                loadGameMenu.Add(loadLabel);

                loadScroll = new ScrollView() { name = "LoadGamesScroll" };
                loadGameMenu.Add(loadScroll);


                VisualElement loadButtonElement = new VisualElement();
                loadButtonElement.style.flexDirection = FlexDirection.Row;
                loadGameMenu.Add(loadButtonElement);

                loadAndClose = new Button() { name = "LoadAndClose", text = "Load" };
                loadButtonElement.Add(loadAndClose);
                loadAndClose.SetEnabled(false);

                returnFromLoadMenu = new Button() { name = "ReturnFromLoad", text = "Cancel" };
                loadButtonElement.Add(returnFromLoadMenu);

                loadGameMenu.style.display = DisplayStyle.None;


                optionsMenu = new VisualElement() { name = "OptionsMenu" };
                optionsMenu.style.position = Position.Absolute;
                optionsMenu.style.left = optionsMenu.style.right = optionsMenu.style.top = optionsMenu.style.bottom = 0;
                screenElement.Add(optionsMenu);

                var optionsLabel = new Label("Options");
                optionsMenu.Add(saveLabel);

                optionsScroll = new ScrollView() { name = "OptionsScroll" };
                optionsMenu.Add(optionsScroll);


                VisualElement optionsButtonElement = new VisualElement();
                optionsButtonElement.style.flexDirection = FlexDirection.Row;
                optionsMenu.Add(optionsButtonElement);

                applyOptionsAndClose = new Button() { name = "ApplyOptionsAndClose", text = "Apply" };
                optionsButtonElement.Add(applyOptionsAndClose);
                applyOptionsAndClose.SetEnabled(false);

                returnFromOptionsMenu = new Button() { name = "ReturnFromOptions", text = "Cancel" };
                optionsButtonElement.Add(returnFromOptionsMenu);

                optionsMenu.style.display = DisplayStyle.None;

            }

            if (screenElement != null) { screenElement.style.width = screenSize.x; screenElement.style.height = screenSize.y; }
            
            if (cabinetElement != null) { cabinetElement.style.width = cabinetSize.x; cabinetElement.style.height = cabinetSize.y; }

            if (gameTitle != null) { gameTitle.text = game.name; }

            if (startGame != null) { startGame.clicked += StartGame; }
            if (saveGame != null) { saveGame.clicked += OpenSaveGameMenu; }
            if (loadGame != null) { loadGame.clicked += OpenLoadGameMenu; }
            if (quitGame != null) { quitGame.clicked += QuitGame; }
            if (returnToGame != null) { returnToGame.clicked += ContinueGame; }
            if (options != null) { options.clicked += OpenOptionsMenu; }


            if(saveGameMenu != null)
            {
                if (!game.CanSave) { saveGame.style.display = DisplayStyle.None; }
                else
                {
                    if (saveGameName != null) { saveGameName.RegisterValueChangedCallback(SaveGameNameChange); }
                    if (saveScroll != null) { BuildSaveGamesScroll(); }
                    if (saveAndClose != null) { saveAndClose.clicked += SaveAndCloseMenu; }
                    if (returnFromSaveMenu != null) { returnFromSaveMenu.clicked += CloseSaveMenu; }
                }
            }

            if (loadGameMenu != null)
            {
                if (!game.CanLoad) { loadGame.style.display = DisplayStyle.None; }
                else
                {
                   
                    if (loadScroll != null) { BuildLoadGamesScroll(); }
                    if (loadAndClose != null) { loadAndClose.clicked += LoadAndCloseMenu; }
                    if (returnFromLoadMenu != null) { returnFromLoadMenu.clicked += CloseLoadMenu; }
                }
            }

            if(optionsMenu != null)
            {
                if (optionsScroll != null) { BuildOptionsScroll(); }
                if (applyOptionsAndClose != null) { applyOptionsAndClose.clicked += ApplyOptionsAndCloseMenu; }
                if (returnFromOptionsMenu != null) { returnFromOptionsMenu.clicked += CloseOptionsMenu; }
            }
        }

        #region Root Menu Interactions

        private void ContinueGame()
        {
            game.ContinueGame();
        }

        private void QuitGame()
        {
            game.QuitGame();
        }

        private void StartGame()
        {
            game.StartGame();
        }

        private void OpenSaveGameMenu()
        {
            saveGameMenu.style.display = DisplayStyle.Flex;
        }
        private void OpenLoadGameMenu()
        {
            loadGameMenu.style.display = DisplayStyle.Flex;
        }

        private void OpenOptionsMenu()
        {
            optionsMenu.style.display = DisplayStyle.Flex;
        }

        #endregion

        #region Save Menu Methods

        private void CloseSaveMenu()
        {
            saveGameName.value = "";
            saveGameMenu.style.display = DisplayStyle.None;
        }

        private void SaveAndCloseMenu()
        {
            throw new NotImplementedException();
        }

        private void BuildSaveGamesScroll()
        {
            throw new NotImplementedException();
        }

        private void SaveGameNameChange(ChangeEvent<string> evt)
        {
            if (string.IsNullOrWhiteSpace(evt.newValue)) { saveAndClose.SetEnabled(false); }
        }

        #endregion

        #region Load Menu Methods

        private void CloseLoadMenu()
        {
            loadGameMenu.style.display = DisplayStyle.None;
        }

        private void LoadAndCloseMenu()
        {
            throw new NotImplementedException();
        }

        private void BuildLoadGamesScroll()
        {
            throw new NotImplementedException();
        }

        #endregion

        #region Options Menu Methods

        private void BuildOptionsScroll()
        {
            throw new NotImplementedException();
        }

        private void ApplyOptionsAndCloseMenu()
        {
            throw new NotImplementedException();
        }

        private void CloseOptionsMenu()
        {
            throw new NotImplementedException();
        }

        #endregion

    }
}