using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

using Ideam;

using Ideam.Axiology.Aesthetics.The_Arts.Ludology;
using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

using Ideam.Metaphysics.Ontology.Existence;

using Ideam.Science.Natural.Cosmology;
using Ideam.Science.Formal.Mathematics.Information.Cryptography;
using Ideam.Science.Formal.Linguistics;
using Ideam.Science.Formal.Systems;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;
using Ideam.Unity.Science.Applied.Audio;
using Ideam.Unity.Science.Formal.Systems;

namespace Ideam.Unity.Science.Natural.Cosmology
{
    public class UnityCosmos : MonoBehaviour, ICosmos, ISystem, ILanguageProvider
    { 
        
        /// <summary>
        /// This is almost exclusively going to be the MainCamera.
        /// </summary>
        public Camera cosmicCamera;

        public Jukebox jukebox;

        public UnityGame[] games = new UnityGame[0];
        
        /// <summary>
        /// Which game, from the Games list, should the app play, on startup? If this is less than zero or greater than the last games ID number, 
        /// no game is automatically started.
        /// </summary>
        public int startupGame = -1;
        
        /// <summary>
        /// Cosmic Objects are not unloaded when scenes are loaded or unloaded. When adding objects to it, at runtime, you should also then call ValidateCosmicObjects.
        /// </summary>
        public List<GameObject> cosmicObjects = new List<GameObject>();

        /// <summary>
        /// The primary language asset used by the app for semantic representations.
        /// </summary>
        public UnityLanguageAsset languageAsset;

        protected UnityGame rootActiveGame;
        
        protected Scene launchScene;

        protected UnityUser temporaryUser, localUser;

        public UnityUser LocalUser { get { return localUser; } }

        #region MonoBehaviour Implementation

        protected virtual void Awake()
        {
            launchScene = SceneManager.GetActiveScene();
            temporaryUser = new GameObject("Temp User").AddComponent<UnityUser>();
        }


        protected virtual void Start()
        {
            if (startupGame >= 0 && startupGame < games.Length)
            {
                StartCoroutine(WaitThenLoadStartupGame());
            }
        }

        //Use to enable cosmic-level managers that should be inaccessible during play. Typically called
        //when the root active game is paused or stopped.
        protected virtual void OnEnable()
        {

        }

        //Disables cosmic-level managers. Typically called when the root active game enters play state.
        protected virtual void OnDisable()
        {

        }

        protected virtual void OnDestroy()
        {

            Application.Quit();
        }
        #endregion


        #region ICosmos Implementation

        public T GetGame<T>(int id) where T : IGame
        {
            if (id < 0 || id >= games.Length) { return default; }

            if (games[id] is T tGame) { return tGame; }

            return default;
        }

        public T[] GetGames<T>() where T : IGame
        {
            List<T> tGames = new List<T>();
            for (int i = 0; i < games.Length; i++)
            {
                if (games[i] is T tgame)
                {
                    tGames.Add(tgame);
                }
            }
            return tGames.ToArray();
        }

        public T GetImagination<T>(int id) where T : IImagination
        {
            throw new System.NotImplementedException();
        }

        public T[] GetImaginations<T>() where T : IImagination
        {
            throw new System.NotImplementedException();
        }

        public T GetNarrative<T>(int id) where T : INarrative
        {
            if (id < 0 || id >= games.Length) { return default; }
            if (games[id].unityNarrative && games[id].unityNarrative.narrative is T tNarrative)
            { return tNarrative; }

            return default;
        }

        public T[] GetNarratives<T>() where T : INarrative
        {
            List<T> tNarratives = new List<T>();
            for (int i = 0; i < games.Length; i++)
            {
                if (games[i].unityNarrative && games[i].unityNarrative.narrative is T tNarrative)
                { tNarratives.Add(tNarrative); }
            }
            return tNarratives.ToArray();
        }

        public T[] GetRealities<T>() where T : IReality
        {
            List<T> tGames = new List<T>();
            for (int i = 0; i < games.Length; i++)
            {
                if (games[i] is T tgame)
                { tGames.Add(tgame); }
            }
            return tGames.ToArray();
        }

        public T GetReality<T>(int id) where T : IReality
        {
            if (id < 0 || id >= games.Length) { return default; }

            if (games[id] is T tGame) { return tGame; }

            return default;
        }

        public T GetUniverse<T>(int id) where T : IUniverse
        {
            if (id < 0 || id >= games.Length) { return default; }

            if (games[id] is T tGame) { return tGame; }

            return default;
        }

        public T[] GetUniverses<T>() where T : IUniverse
        {
            List<T> tGames = new List<T>();
            for (int i = 0; i < games.Length; i++)
            {
                if (games[i] is T tgame)
                { tGames.Add(tgame); }
            }
            return tGames.ToArray();
        }

        #endregion

        public virtual int AddGame(UnityGame gameToAdd)
        {
            int gameID = GetGameID(gameToAdd);
            if(gameID < 0)
            {
                gameID = games.Length;
                System.Array.Resize(ref games, gameID + 1);
                games[gameID] = gameToAdd;
            }
            return gameID;
        }

        public virtual int GetGameID(UnityGame game)
        {
            for(int i =0; i < games.Length; i++)
            {
                if(games[i] == game) { return i; }
            }
            return -1;
        }
        public virtual void LoadGame(int id)
        {
            if (!games[id].IsLoaded) { games[id].LoadGame(this); }
        }

        IEnumerator WaitThenLoadStartupGame()
        {
            yield return new WaitForSeconds(10f);
            LoadGame(startupGame);
            yield break;
        }

        protected virtual void UnloadLaunchScene()
        {
            ValidateCosmicObjects();
            var launchUnloaderHandler = SceneManager.UnloadSceneAsync(launchScene);
            launchUnloaderHandler.completed += LaunchSceneUnloaded;
        }

        private void LaunchSceneUnloaded(AsyncOperation obj)
        {
        }

        public virtual void ValidateCosmicObjects()
        {
            for(int i =0; i < cosmicObjects.Count; i++)
            {
                DontDestroyOnLoad(cosmicObjects[i]);
            }
        }

        #region ILanguageProvider Implementation
        public virtual T GetLanguage<T>() where T : ILanguage
        {
            if (!languageAsset) { return default; }

            UnityLanguageAsset.CurrentLanguageAsset = languageAsset;
            
            if(languageAsset is T tLanguage)
            {
                return tLanguage;
            }
            return default;
        }

        public virtual UnityLanguageAsset GetLanguage()
        {
            if (!languageAsset) { return default; }

            UnityLanguageAsset.CurrentLanguageAsset = languageAsset;
            return languageAsset;
        }

        #endregion

        #region ISystem Implementation 
        public T GetEnvironment<T>() where T : IPlace
        {
            throw new System.NotImplementedException();
        }

        public T GetUser<T>(int id) where T : IUser
        {
            throw new System.NotImplementedException();
        }

        public T[] GetUsers<T>() where T : IUser
        {
            throw new System.NotImplementedException();
        }

        public T Security<T>() where T : ISecurity
        {
            throw new System.NotImplementedException();
        }
        #endregion

    }
}
