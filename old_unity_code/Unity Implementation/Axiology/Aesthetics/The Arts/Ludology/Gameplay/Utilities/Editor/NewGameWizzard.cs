using System;
using System.Collections.Generic;
using System.IO;

using UnityEditor;
using UnityEditor.SceneManagement;

using UnityEngine;
using UnityEngine.SceneManagement;

using Cinemachine;

using Ideam.Editor;

using Ideam.Unity.Science.Natural.Cosmology;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

using Ideam.Unity.Semantics;
using Ideam.Unity.Science.Applied.Audio;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.UI;

namespace Ideam.Unity
{
    //The goal here is to create a ProceduralGraph - not a variant class, but just a procedural graph.
    //All the nodes are public static methods on various things, marked with a NodeAction attribute e.g. 
    // [NodeAction()]
    // public static void NodeActionOfSomeSort(Inputs, out Outputs){}

    public class NeoGameWizzard : EditorWindow
    {
        [MenuItem("Tools/Ideam/Neo-Game-Wizzard")]
        static void OpenNeoGameWizzard()
        {
            if (!gameWizzard) gameWizzard = GetWindow<NeoGameWizzard>();

        }
        static NeoGameWizzard gameWizzard;

        UnityGameWizardUI ui;
        private void OnEnable()
        {
            ui = CreateInstance<UnityGameWizardUI>();
            rootVisualElement.Add( ui.OpenUserInterface());
        }
    }

    class NewGameWizzard : ScriptableWizard
    {
        public string newGameName;
        public SceneAsset cosmosScene;
        public bool createStandardSubFolders, createSemanticAssets, createNarrativeAsset, 
                    createCosmosScene, createAndOpenGameScene, createGlobalJukebox, createMusicZone;

        string gamesPath;

        [MenuItem("Tools/Ideam/New Game Wizzard", false, 2000)]
        static void CreateWizard()
        {

            var newGameWizard = DisplayWizard<NewGameWizzard>("Create New Game");
            newGameWizard.gamesPath = "Assets/Games";
        }

        private void OnWizardUpdate()
        {
            isValid = ValidateNewGameName();
        }

        private void OnWizardCreate()
        {
            string newGameFolderGUID = AssetDatabase.CreateFolder(gamesPath, newGameName);
            string newGameFolderPath = AssetDatabase.GUIDToAssetPath(newGameFolderGUID);
            string newGameSceneFolderPath = string.Empty;

            //1. Create the UnityGame asset
            UnityGame newGame = CreateInstance<UnityGame>();

            UnitySemanticManager gameSemanticManager = null;
            UnityLanguageAsset gameLanguageAsset = null;

            newGame.name = newGameName;
            AssetDatabase.CreateAsset(newGame, newGameFolderPath + "/" + newGameName + ".asset");

            //2. Create various useful sub-folders
            if (createStandardSubFolders)
            {
                AssetDatabase.CreateFolder(newGameFolderPath, "Materials");
                AssetDatabase.CreateFolder(newGameFolderPath, "Models");
                AssetDatabase.CreateFolder(newGameFolderPath, "Prefabs");
                newGameSceneFolderPath = newGameFolderPath + "/Scenes/";
                AssetDatabase.CreateFolder(newGameFolderPath, "Scenes");
                AssetDatabase.CreateFolder(newGameFolderPath, "Scripts");
                AssetDatabase.CreateFolder(newGameFolderPath, "Settings");
                AssetDatabase.CreateFolder(newGameFolderPath, "Textures");
            }
            if (createSemanticAssets)
            {
                gameSemanticManager = CreateInstance<UnitySemanticManager>();
                gameSemanticManager.name = newGameName + " Semantics";
                AssetDatabase.CreateAsset(gameSemanticManager, newGameFolderPath + "/" + gameSemanticManager.name + ".asset");
               
                gameLanguageAsset = CreateInstance<UnityLanguageAsset>();
                gameLanguageAsset.name = newGameName + " Language";
                AssetDatabase.CreateAsset(gameLanguageAsset, newGameFolderPath + "/" + gameLanguageAsset.name + ".asset");
                
                gameLanguageAsset.semanticManager = gameSemanticManager;
                EditorUtility.SetDirty(gameLanguageAsset);

                newGame.languageAsset = gameLanguageAsset;
                EditorUtility.SetDirty(newGame);
            }

            //3. Create Narrative
            if (createNarrativeAsset)
            {
                AssetDatabase.CreateFolder(newGameFolderPath, "Narrative");

                UnityNarrative gameNarrative = CreateInstance<UnityNarrative>();
                gameNarrative.name = newGameName + " Narrative";
                
                AssetDatabase.CreateAsset(gameNarrative, newGameFolderPath + "/Narrative/" + gameNarrative.name + ".asset");
                newGame.unityNarrative = gameNarrative;
                EditorUtility.SetDirty(newGame);
            }

            if(cosmosScene != null)
            {
                string cScenePath = AssetDatabase.GetAssetPath(cosmosScene);
                Scene cScene = EditorSceneManager.GetSceneByPath(cScenePath);
                bool unloadCosmicScene = false;
                if (!cScene.IsValid())
                {
                    cScene = EditorSceneManager.OpenScene(cScenePath, OpenSceneMode.Additive);
                    unloadCosmicScene = true;
                }
                AddGameToCosmosScene(cScene, newGame, unloadCosmicScene);
                
            } 
            else if (createCosmosScene)
            {
                Scene cScene = EditorExtension.SetupCosmosScene(newGameFolderPath + "/" + newGameName + "_Cosmos.unity").scene;
                AddGameToCosmosScene(cScene, newGame);
            }

            if (createAndOpenGameScene && createStandardSubFolders)
            {
                string newGameSceneName = newGameName;
                string newGameScenePath = newGameSceneFolderPath + newGameSceneName + ".unity";
                bool newSceneUpdated = false;
                
                EditorExtension.SceneSetupInfo setupScene = EditorExtension.SetupGameScene(newGame, newGameSceneName, newGameScenePath);
                
                if (createSemanticAssets)
                {
                    setupScene.gameBoard.languageAsset = gameLanguageAsset;
                    newSceneUpdated = true;
                }

                if (createGlobalJukebox)
                {
                    Transform cameraTransform = setupScene.mainCamera.transform;
                    
                    GameObject jukeboxObject = new GameObject("Jukebox", typeof(Jukebox));
                    Jukebox jukebox = jukeboxObject.GetComponent<Jukebox>();
                    jukeboxObject.transform.SetParent(cameraTransform, false);
                    jukeboxObject.transform.localEulerAngles = jukeboxObject.transform.localPosition = Vector3.zero;
                    jukeboxObject.transform.localScale = Vector3.one;

                    GameObject sourceAObject = new GameObject("Source A", typeof(AudioSource));
                    AudioSource sourceA = sourceAObject.GetComponent<AudioSource>();
                    sourceAObject.transform.SetParent(jukeboxObject.transform, false);
                    sourceAObject.transform.localEulerAngles = sourceAObject.transform.localPosition = Vector3.zero;
                    sourceAObject.transform.localScale = Vector3.one;

                    GameObject sourceBObject = new GameObject("Source B", typeof(AudioSource));
                    AudioSource sourceB = sourceBObject.GetComponent<AudioSource>();
                    sourceBObject.transform.SetParent(jukeboxObject.transform, false);
                    sourceBObject.transform.localEulerAngles = sourceBObject.transform.localPosition = Vector3.zero;
                    sourceBObject.transform.localScale = Vector3.one;

                    jukebox.sourceA = sourceA; jukebox.sourceObjectA = sourceAObject;
                    jukebox.sourceB = sourceB; jukebox.sourceObjectB = sourceBObject;

                    if (createMusicZone)
                    {
                        MusicZone defaultMusicZone = new GameObject("Default Music Zone").AddComponent<MusicZone>();
                        defaultMusicZone.jukebox = jukebox;

                        TriggerSensor zoneTrigger = defaultMusicZone.gameObject.AddComponent<TriggerSensor>();
                        defaultMusicZone.zoneTrigger = zoneTrigger;

                        BoxCollider zoneBounds = defaultMusicZone.gameObject.AddComponent<BoxCollider>();
                        zoneBounds.isTrigger = true;
                        zoneBounds.size = Vector3.one * 10;
                    }

                    newSceneUpdated = true;
                }

                if (newSceneUpdated) { EditorSceneManager.SaveScene(setupScene.scene); }
            }

            AssetDatabase.SaveAssets();
        }
        
        private void AddGameToCosmosScene(Scene cosmosScene, UnityGame gameToAdd, bool unload = false)
        {
            UnityCosmos[] cosmoses = cosmosScene.GetComponents<UnityCosmos>(true);
            if (cosmoses.Length > 0)
            {
                List<UnityGame> cosmicGames = new List<UnityGame>(cosmoses[0].games);
                cosmicGames.Add(gameToAdd);
                cosmoses[0].games = cosmicGames.ToArray();
                EditorUtility.SetDirty(cosmoses[0]);
                EditorSceneManager.SaveScene(cosmosScene);
            }
            if (unload)
            {
                EditorSceneManager.CloseScene(cosmosScene, true);
            }
        }

        private bool ValidateNewGameName()
        {
            if (string.IsNullOrEmpty(newGameName) || string.IsNullOrWhiteSpace(newGameName)) { errorString = "No valid game name entered."; return false; }
            string gameFolder = gamesPath +"/"+ newGameName;
            if (AssetDatabase.IsValidFolder(gameFolder)) { errorString = "Game folder already exists."; return false; }

            errorString = "";
            return true;
        }
        
    }
}
