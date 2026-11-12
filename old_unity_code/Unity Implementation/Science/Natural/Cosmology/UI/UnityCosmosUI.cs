using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;
using Ideam.Unity.Science.Applied.Audio;

namespace Ideam.Unity.Science.Natural.Cosmology
{
    [UserInterface(typeof(UnityCosmos))]
    public class UnityCosmosUI : UserInterface<UnityCosmos>
    {
        public VisualTreeAsset selectedGameUI;
        protected ObjectSelectorField cosmicCameraField, jukeboxField, languageAssetField, gameAssetField;
        protected DropList gameSelector, cosmicObjectSelector;
        protected VisualElement selectedGameElement, selectedCosmicObjectElement;
        protected Toggle isStartupGame;
        protected Button loadGameButton;

        protected int selectedGameID, selectedCosmicObjectID;
        public override void SubscribeToObjectEvents()
        {
        }

        public override void UnsubscribeFromObjectEvents()
        {
        }

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }

            cosmicCameraField = root.Q<ObjectSelectorField>("CosmicCameraField");
            cosmicCameraField.Initialize<Camera>(SetCosmicCamera, sourceObject.cosmicCamera);

            jukeboxField = root.Q<ObjectSelectorField>("JukeboxField");

            jukeboxField.Initialize<Jukebox>(SetJukebox, sourceObject.jukebox, null, ObjectSelectorField.ObjectSearchMode.Scene);

            languageAssetField = root.Q<ObjectSelectorField>("LanguageAssetField");
            languageAssetField.Initialize<UnityLanguageAsset>(SetLanguageAsset, sourceObject.languageAsset);


            selectedCosmicObjectElement = root.Q<VisualElement>("SelectedCosmicObjectElement");

            cosmicObjectSelector = root.Q<DropList>("CosmicObjectSelector");
            string[] cosmicObjectNames = new string[sourceObject.cosmicObjects.Count];
            for (int g = 0; g < sourceObject.cosmicObjects.Count; g++)
            {
                if (sourceObject.cosmicObjects[g] == null) { cosmicObjectNames[g] = "(None)"; continue; }

                cosmicObjectNames[g] = sourceObject.cosmicObjects[g].name;
            }


            cosmicObjectSelector.BuildList(cosmicObjectNames);
            cosmicObjectSelector.OnSelect += SelectCosmicObject;
            cosmicObjectSelector.OnAdd += AddCosmicObject;


            selectedGameElement = root.Q<VisualElement>("SelectedGameElement");

            gameSelector = root.Q<DropList>("GameSelector");
            string[] gameNames = new string[sourceObject.games.Length];
            for (int g = 0; g < sourceObject.games.Length; g++)
            {
                if (sourceObject.games[g] == null) { gameNames[g] = "(None)"; continue; }

                gameNames[g] = sourceObject.games[g].name;
            }


            gameSelector.BuildList(gameNames);
            gameSelector.OnSelect += SelectGame;
            gameSelector.OnAdd += AddGame;

            return true;
        }

        private void AddCosmicObject(int obj)
        {
            throw new NotImplementedException();
        }

        private void SelectCosmicObject(int obj)
        {
            selectedCosmicObjectID = obj;
            selectedCosmicObjectElement.Clear();

            ObjectSelectorField cosmicObjectField = new ObjectSelectorField("Cosmic Object #" + (obj + 1).ToString());
            
            cosmicObjectField.Initialize<GameObject>(GetCosmicObject, sourceObject.cosmicObjects[obj]);
            selectedCosmicObjectElement.Add(cosmicObjectField);

        }

        private void GetCosmicObject(GameObject selection)
        {
            selectedCosmicObjectID = cosmicObjectSelector.Selected;
            sourceObject.cosmicObjects[selectedGameID] = selection;
            string[] cosmicObjectNames = new string[sourceObject.cosmicObjects.Count];
            for (int g = 0; g < sourceObject.cosmicObjects.Count; g++)
            {
                if (sourceObject.cosmicObjects[g] == null) { cosmicObjectNames[g] = "(None)"; continue; }

                cosmicObjectNames[g] = sourceObject.cosmicObjects[g].name;
            }


            cosmicObjectSelector.BuildList(cosmicObjectNames);
        }

        private void AddGame(int obj)
        {
            System.Array.Resize(ref sourceObject.games, sourceObject.games.Length + 1);
            //I entered a name and here's this whole rigamarol. Do I want to try and find that game as an addressable? Not right now.
            //But someday...
            SelectGame(obj);
        }

        private void SetCosmicCamera(Camera selection)
        {
            sourceObject.cosmicCamera = selection;
        }

        private void SetJukebox(Jukebox selection)
        {
            sourceObject.jukebox = selection;
        }

        private void SetLanguageAsset(UnityLanguageAsset selection)
        {
            sourceObject.languageAsset = selection;
        }

        private void SetGameAsset(UnityGame selection)
        {
            selectedGameID = gameSelector.Selected;
            sourceObject.games[selectedGameID] = selection;
            string[] gameNames = new string[sourceObject.games.Length];
            for (int g = 0; g < sourceObject.games.Length; g++)
            {
                if (sourceObject.games[g] == null) { gameNames[g] = "(None)"; continue; }

                gameNames[g] = sourceObject.games[g].name;
            }


            gameSelector.BuildList(gameNames);
        }

        private void SelectGame(int id)
        {
            selectedGameID = id;
            selectedGameElement.Clear();
            if (selectedGameUI != null)
            {
                VisualElement gameElement = selectedGameUI.CloneTree();
                selectedGameElement.Add(gameElement);

                gameAssetField = gameElement.Q<ObjectSelectorField>("GameAssetField");
                gameAssetField.Initialize<UnityGame>(SetGameAsset, sourceObject.games[id], null, ObjectSelectorField.ObjectSearchMode.Resources);

                isStartupGame = gameElement.Q<Toggle>("IsStartupGameToggle");
                isStartupGame.value = sourceObject.startupGame == id;
                isStartupGame.RegisterValueChangedCallback(evt => 
                {
                    if (evt.newValue)
                    {
                        sourceObject.startupGame = id;
                    }
                    else
                    {
                        sourceObject.startupGame = -1;
                    }
                });

                loadGameButton = gameElement.Q<Button>("LoadButton");

                loadGameButton.clicked += () => { sourceObject.LoadGame(id); };
            }
        }
        public override bool CloseUserInterface()
        {
            return base.CloseUserInterface();
        }
    }
}