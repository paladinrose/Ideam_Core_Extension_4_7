using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.AddressableAssets;
using UnityEngine.ResourceManagement.AsyncOperations;
using UnityEngine.ResourceManagement.ResourceProviders;
using UnityEngine.SceneManagement;

using Ideam.Axiology.Aesthetics.The_Arts.Ludology;

using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Metaphysics.Ontology.Identity;

using Ideam.Science.Natural.Physics;
using Ideam.Science.Natural.Cosmology;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Linguistics;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology;
using Ideam.Unity.Science.Natural.Cosmology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    
    /// <summary>
    /// The UnityGame is a combination Game, Universe, and Reality. 
    /// </summary>
    [CreateAssetMenu(fileName = "New Game", menuName = "Ideam/Game")]
    public class UnityGame : ScriptableObject, IGame, IUniverse, IReality, ILanguageProvider
    {
        public class UnityGameScene
        {
            public Scene scene;
            public GameObject[] sceneObjects;
            public AsyncOperationHandle<SceneInstance> loadingHandle;
            
            public List<UnityGameBoard> gameBoards = new List<UnityGameBoard>();

            public List<UnityGamePiece> gamePieces = new List<UnityGamePiece>();

            public List<UnityGamePlayer> gamePlayers = new List<UnityGamePlayer>();
        }

        

        [Serializable]
        public struct ValidGamePieceProperty
        {
            public int sememe;
            public Int upperLimit, lowerLimit;
        }

        [Serializable]
        public struct ValidGamePieceResource
        {
            public int sememe;
            public bool constrainCurrentLimitsToWithinMaxLimits;
            public Int currentUpperLimit, currentLowerLimit, maxUpperLimit, maxLowerLimit;
        }

        [Serializable]
        public struct ValidInteraction
        {
            public int sememe;
            public Int upperLimit, lowerLimit;
        }

        public virtual event Action<UnityGameScene> OnLoadStart, OnLoadComplete;

        public virtual event Action<IGame> OnStart, OnPause, OnContinue, OnQuit;

        public AssetReference[] cosmicReferences, 
                                globalReferences, 
                                sceneReferences;

        public int gameStartScene;
        
        
        protected UnityGameScene[] scenes = new UnityGameScene[0];
        
        public ValidGamePieceProperty[] validGamePieceProperties;
        public int defaultPropertyLowerLimit, defaultPropertyUpperLimit;

        public ValidGamePieceResource[] validGamePieceResources;
        public bool defaultResourceCurrentLimitConstraint;
        public int defaultResourceCurrentLowerLimit, defaultResourceCurrentUpperLimit, defaultResourceMaxLowerLimit, defaultResourceMaxUpperLimit;

        public ValidInteraction[] validInteractions;
        
        public uint dimensions = 4;
        public UnityGameField[] gameFields;

        public UnityNarrative unityNarrative;
        public UnityLanguageAsset languageAsset;

        protected bool isLoaded;
        public virtual bool IsLoaded { get { return isLoaded; } }

        protected bool canSave;
        public virtual bool CanSave { get { return canSave; } }

        protected bool canLoad;
        public virtual bool CanLoad { get { return canLoad; } }

        protected List<UnityGameBoard> gameBoards = new List<UnityGameBoard>();
        protected List<UnityGamePiece> gamePieces = new List<UnityGamePiece>();
        protected List<UnityGamePlayer> gamePlayers = new List<UnityGamePlayer>();

        protected UnityCosmos cosmos;
        public UnityCosmos Cosmos { get { return cosmos; } }

        public virtual void LoadGame(UnityCosmos cosmos)
        {
            if (IsLoaded) { return; }

            this.cosmos = cosmos;
            
            scenes = new UnityGameScene[sceneReferences.Length];
            LoadScene(gameStartScene);
            
        }

        public virtual void LoadScene(int id) 
        {
            if(id <0 || id >= scenes.Length || scenes[id] != null) { return; }

            scenes[id] = new UnityGameScene();
            scenes[id].loadingHandle = sceneReferences[id].LoadSceneAsync(LoadSceneMode.Additive, true, 0);
            scenes[id].loadingHandle.Completed += (sceneInstance) => { OnNewSceneLoaded(sceneInstance, id); };
            OnLoadStart?.Invoke(scenes[id]);
        }

        protected virtual void OnNewSceneLoaded(AsyncOperationHandle<SceneInstance> sceneInstance, int id)
        {
            Scene newScene = sceneInstance.Result.Scene;
            scenes[id].scene = newScene;

            GameObject[] sceneRoots = newScene.GetRootGameObjects();
            scenes[id].sceneObjects = sceneRoots;
            UnityGameBoard firstBoard = null;
            for(int i =0; i < sceneRoots.Length; i++)
            {
                UnityGameBoard[] sceneboards = sceneRoots[i].GetComponentsInChildren<UnityGameBoard>();
                scenes[id].gameBoards.AddRange(sceneboards);

                UnityGamePiece[] scenePieces = sceneRoots[i].GetComponentsInChildren<UnityGamePiece>();
                scenes[id].gamePieces.AddRange(scenePieces);

                //Do management on GameBoards and GamePieces here.
                if (firstBoard == null && sceneboards.Length > 0)
                {
                    firstBoard = sceneboards[0]; 

                }
            }

            gameBoards.AddRange(scenes[id].gameBoards);
            gamePieces.AddRange(scenes[id].gamePieces);

            SceneManager.SetActiveScene(newScene);

            //Do scene finalization here.

            firstBoard.BoardAwake();

            //LightProbes.TetrahedralizeAsync();

            OnLoadComplete?.Invoke(scenes[id]);
        }

        public virtual void UnloadScene(int id) 
        {
            //Do management on GameBoards and Pieces, etc.
            UnityGameScene scene = scenes[id];
            for(int i = 0; i < scene.gameBoards.Count; i++)
            {
                if (gameBoards.Contains(scene.gameBoards[i]))
                {
                    gameBoards.Remove(scene.gameBoards[i]);
                }
            }

            for (int i = 0; i < scene.gamePieces.Count; i++)
            {
                scene.gamePieces[i].GameQuit(this);
                scene.gamePieces[i].UnlinkFromGame();
                if (gamePieces.Contains(scene.gamePieces[i]))
                {
                    gamePieces.Remove(scene.gamePieces[i]);
                }
            }

            if (sceneReferences[id].OperationHandle.IsValid())
            {
                sceneReferences[id].UnLoadScene();
            }
#if UNITY_EDITOR
            else
            {
                SceneManager.UnloadSceneAsync(sceneReferences[id].editorAsset.name);
            }
#endif

            scenes[id] = null;
        }

        #region IGame Implementation

        #region Game State

        //[SerializeField]
        public GamePhase gamePhase;

        public virtual GamePhase GamePhase { get { return gamePhase; } protected set { gamePhase = value; } }

        public virtual void StartGame()
        {
            GamePhase = GamePhase.Setup;
            //StartCoroutine(GameSetup());
            OnStart?.Invoke(this);
        }

        public virtual void PauseGame()
        {
            GamePhase = GamePhase.Paused;
            OnPause?.Invoke(this);
        }

        public virtual void ContinueGame()
        {
            GamePhase = GamePhase.Play;
            OnContinue?.Invoke(this);
        }

        public virtual void QuitGame()
        {
            GamePhase = GamePhase.Resolution;
            OnQuit?.Invoke(this);
            //StartCoroutine(WaitForQuit());
        }


        public virtual IEnumerator GameSetup()
        {
            GamePhase = GamePhase.Play;

            yield break;
        }



        #endregion

        public virtual T GetGameBoard<T>(int id) where T : IGameBoard
        {
            if (id < 0 || id >= gameBoards.Count) { return default; }

            if (gameBoards[id] is T tGameBoard) { return tGameBoard; }

            return default;
        }

        public virtual T[] GetGameBoards<T>() where T : IGameBoard
        {
            List<T> tGameBoards = new List<T>();
            for (int i = 0; i < gameBoards.Count; i++)
            {
                if (gameBoards[i] is T tGameBoard)
                {
                    tGameBoards.Add(tGameBoard);
                }
            }
            return tGameBoards.ToArray();
        }

        public virtual T GetGamePiece<T>(int id) where T : IGamePiece
        {
            if (id < 0 || id >= gamePieces.Count) { return default; }

            if (gamePieces[id] is T tGamePiece) { return tGamePiece; }

            return default;
        }

        public virtual T[] GetGamePieces<T>() where T : IGamePiece
        {
            List<T> tGamePieces = new List<T>();
            for (int i = 0; i < gamePieces.Count; i++)
            {
                if (gamePieces[i] is T tGamePiece)
                {
                    tGamePieces.Add(tGamePiece);
                }
            }
            return tGamePieces.ToArray();
        }

        public virtual T GetGamePlayer<T>(int id) where T : IGamePlayer
        {
            if (id < 0 || id >= gamePlayers.Count) { return default; }

            if (gamePlayers[id] is T tGamePlayer) { return tGamePlayer; }

            return default;
        }

        public virtual T[] GetGamePlayers<T>() where T : IGamePlayer
        {
            throw new NotImplementedException();
        }

        #endregion

        #region Game Rules
        public bool ValidateAction(UnityGameAction actionToValidate)
        {


            return false;
        }
        #endregion


        #region IUniverseImplementation

        public virtual uint NumberOfDimensions { get { return dimensions; } }

        public virtual int GetFieldID<T>(T intrinsicField) where T : IFundamentalField
        {
            if(intrinsicField is UnityGameField utrinsicField)
            {
                for(int i =0; i < gameFields.Length; i++)
                {
                    if(gameFields[i] == utrinsicField) { return i; }
                }
            }
            return -1;
        }

        public virtual T GetField<T>(int fieldID) where T : IFundamentalField
        {
            if (fieldID < 0 || fieldID >= gameFields.Length) { return default; }
            if(gameFields[fieldID] is T tField) { return tField; }
            return default;
        }

        #endregion

        #region IReality Implementation
        
        public virtual T[] GetThings<T>() where T : IThing, IReal
        {
            List<T> things = new List<T>();
            for(int i=0; i < gamePieces.Count; i++)
            {
                if(gamePieces[i] is T tPiece)
                {
                    things.Add(tPiece);
                }
            }
            return things.ToArray();
        }

        public virtual bool IsRealThing<T>(T realityCheck) where T : IThing, IReal
        {
            if (realityCheck is UnityGamePiece realPiece)
            {
                UnityGamePieceProperty[] pieceProperties = realPiece.Properties<UnityGamePieceProperty>();
                if(pieceProperties.Length > 0)
                {
                    bool allValidProperties = true;

                    for(int i =0; i < pieceProperties.Length; i++)
                    {
                        if(IsRealProperty(pieceProperties[i])) { allValidProperties = false; break; }
                    }

                    return allValidProperties;
                }
            }
            return false;
        }

        public T[] GetPlaces<T>() where T : IPlace, IReal
        {
            List<T> tPlaces = new List<T>();

            for(int i =0; i < gameBoards.Count; i++)
            {
                if(gameBoards[i] is T tPlace)
                {
                    tPlaces.Add(tPlace);
                }
            }

            return tPlaces.ToArray();
        }

        public bool IsRealPlace<T>(T realityCheck) where T : IPlace, IReal
        {
            if (realityCheck is UnityGameBoard realBoard)
            {
                return true;
            }
            return false;
        }

        public T[] GetProperties<T>() where T : IProperty
        {
            if (!typeof(T).IsAssignableFrom(typeof(UnityGamePieceProperty))) { return new T[0]; }

            List<T> validProperties = new List<T>();
            for(int i =0; i < validGamePieceProperties.Length; i++)
            {
                UnityGamePieceProperty newProperty = new UnityGamePieceProperty()
                { sememe = validGamePieceProperties[i].sememe,  
                    value = validGamePieceProperties[i].lowerLimit +
                    validGamePieceProperties[i].upperLimit / 2};
                if(newProperty is T tProp) { validProperties.Add(tProp); }
                
            }
            return validProperties.ToArray();
        }

        public bool IsRealProperty<T>(T realityCheck) where T : IProperty
        {
            if (realityCheck is UnityGamePieceProperty pieceProperty)
            {
                for (int j = 0; j < validGamePieceProperties.Length; j++)
                {
                    if (pieceProperty.Sememe() == validGamePieceProperties[j].sememe &&
                       pieceProperty.value <= validGamePieceProperties[j].upperLimit &&
                       pieceProperty.value >= validGamePieceProperties[j].lowerLimit)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        public virtual UnityGamePieceProperty AddRealProperty(string label)
        {
            UnityGamePieceProperty newProp = default;
            
            UnityLanguageAsset lang = GetLanguage();
            if (lang)
            { 
                newProp = new UnityGamePieceProperty() { sememe = lang.GetSemanticID(label), value = defaultPropertyLowerLimit };
            }

            if (!newProp.Equals(default))
            {
                ValidGamePieceProperty realProperty = new ValidGamePieceProperty()
                {
                    lowerLimit = defaultPropertyLowerLimit,
                    sememe = newProp.sememe,
                    upperLimit = defaultPropertyUpperLimit
                };
                List<ValidGamePieceProperty> newValidProps = new List<ValidGamePieceProperty>(validGamePieceProperties);
                newValidProps.Add(realProperty);
                validGamePieceProperties = newValidProps.ToArray();
            }

            return newProp;
        }
        #endregion

        #region ILanguageProvider Implementation
        public virtual T GetLanguage<T>() where T : ILanguage
        {
            if (!languageAsset)
            {
                languageAsset = UnityLanguageAsset.CurrentLanguageAsset;
            }

            if (languageAsset && languageAsset is T tLanguage)
            {
                return tLanguage;
            }
            else
            {
                return cosmos.GetLanguage<T>();
            }
        }

        public virtual UnityLanguageAsset GetLanguage()
        {
            if (!languageAsset)
            {
                languageAsset = UnityLanguageAsset.CurrentLanguageAsset;
            }

            if (languageAsset)  {  return languageAsset; }
            else { return cosmos.GetLanguage(); }
        }
        #endregion
    }
}