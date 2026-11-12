using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AddressableAssets;
using UnityEngine.ResourceManagement.AsyncOperations;

using Ideam.Unity.UI;
using UnityEngine.UIElements;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class UnityGameWizardUI : UserInterface
    {
        public override VisualTreeAsset UXML { get => base.UXML; set => base.UXML = value; }
        protected override void LoadDefaultUXML()
        {
            if (waitingForUXML) { return; }

            waitingForUXML = true;
            string uxmlPath = "Ideam/Unity/Axiology/Aesthetics/The_Arts/Ludology/UnityGameWizard.uxml";
            uxmlHandle = Addressables.LoadAssetAsync<VisualTreeAsset>(uxmlPath);
            uxmlHandle.Completed += UXMLLoadingComplete;

          

        }

        public TabBar wizardTabs;
       
        
        #region Page Field Structs
        
        public struct GamePageFields
        {
            public VisualElement pageElement;
            public TextField newGameNameField;
            public ObjectSelectorField cosmosSceneField;
            public Toggle createStandardFoldersToggle,
                          createCosmosSceneToggle,
                          createSemanticsAssetsToggle,
                          createNarrativeAssetsToggle,
                          createAndOpenGameSceneToggle,
                          createGlobalJukeboxToggle,
                          createMusicZoneToggle;

            public GamePageFields(VisualElement gamePageElement)
            {
                pageElement = gamePageElement;
                newGameNameField = gamePageElement.Q<TextField>("NewGameNameField");
                cosmosSceneField = gamePageElement.Q<ObjectSelectorField>("CosmosSceneField");
                createStandardFoldersToggle = gamePageElement.Q<Toggle>("CreateStandardFoldersToggle");
                createCosmosSceneToggle = gamePageElement.Q<Toggle>("CreateCosmosSceneToggle");
                createSemanticsAssetsToggle = gamePageElement.Q<Toggle>("CreateSemanticsAssetsToggle");
                createNarrativeAssetsToggle = gamePageElement.Q<Toggle>("CreateNarrativeAssetsToggle");
                createAndOpenGameSceneToggle = gamePageElement.Q<Toggle>("CreateAndOpenGameSceneToggle");
                createGlobalJukeboxToggle = gamePageElement.Q<Toggle>("CreateGlobalJukeboxToggle");
                createMusicZoneToggle = gamePageElement.Q<Toggle>("CreateMusicZoneToggle");
            }
        }
        
        #endregion

        public GamePageFields gamePageFields;

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }

            wizardTabs = root.Q<TabBar>();

            VisualElement gamePageElement = ui.Q<VisualElement>("GamePage");
            if (gamePageElement != null)
            {
                gamePageElement.style.display = DisplayStyle.Flex;
                gamePageFields = new GamePageFields(gamePageElement);
                wizardTabs.AddPage(gamePageElement, "Game");
            }

            return true;
        }

        protected void ExecuteWizard()
        {
        }
    }
}