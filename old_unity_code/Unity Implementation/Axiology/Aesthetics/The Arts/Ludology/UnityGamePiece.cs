using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Axiology.Valuation;
using Ideam.Axiology.Aesthetics.The_Arts.Ludology;
using Ideam.Epistemology.Semantics;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Metaphysics.Ontology.Identity;
using Ideam.Science.Formal.Linguistics;
using Ideam.Science.Formal.Mathematics;

using Ideam.Unity.Semantics;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class UnityGamePiece : MonoBehaviour, IGamePiece, IThing, IReal, ILanguageProvider, IAffinity
    {
        #region Variables, Properties, and Events
        
        public UnitySemanticManager semanticManager;
        public UnityLanguageAsset languageAsset;

        public UnityGamePieceProperty[] properties = new UnityGamePieceProperty[0];
        public UnityGamePieceResource[] resources = new UnityGamePieceResource[0];

        public UnityGame game;
        public UnityGameBoard gameBoard;
        public UnityGameAgent gameAgent;
        public UnityGamePiece[] subPieces = new UnityGamePiece[0];


        public virtual event Action<int> OnResourceExhausted;
        public virtual event Action<int, UnityGamePiece> OnAddSubPiece, OnRemoveSubPiece;

        #endregion

        #region IThing Implementation

        public virtual T[] Properties<T>() where T : IProperty
        {
            List<T> tProperties = new List<T>();
            for(int i =0; i < properties.Length; i++)
            {
                if(properties[i] is T tProperty)
                {
                    tProperties.Add(tProperty);
                }
            }
            return tProperties.ToArray();
        }


        public virtual T[] Resources<T>() where T : IResource
        {
            List<T> tResources = new List<T>();
            for (int i = 0; i <resources.Length; i++)
            {
                if (resources[i] is T tResource)
                {
                    tResources.Add(tResource);
                }
            }
            return tResources.ToArray();
        }

        public virtual void ExhaustResource(int id, Int exhaustion)
        {
            resources[id].value -= exhaustion;
            if (resources[id].value <= 0) { OnResourceExhausted?.Invoke(id); }
        }
        #endregion

        #region IReal Implementation

        public virtual bool SetReality<T>(T reality) where T : IReality
        {
            if(reality is UnityGame realGame)
            {
                return LinkToGame(realGame);
            }
            return false;
        }

        public virtual T GetReality<T>() where T : IReality
        {
            if (game is T tGame) return tGame;

            return default;
        }

        #endregion

        #region IGamePiece Implementation

        public virtual T GetActiveGame<T>() where T : IGame
        {
            if(game is T tGame) { return tGame; }
            return default;
        }

        public virtual void GameStart(IGame game)
        {
            if(game is UnityGame uGame)
            {
                if(uGame == this.game)
                {

                }
            }
        }

        public virtual void GamePause(IGame game)
        {
            if (game is UnityGame uGame)
            {
                if (uGame == this.game)
                {

                }
            }
        }

        public virtual void GameContinue(IGame game)
        {
            if (game is UnityGame uGame)
            {
                if (uGame == this.game)
                {

                }
            }
        }

        public virtual void GameQuit(IGame game)
        {
            if (game is UnityGame uGame)
            {
                if (uGame == this.game)
                {

                }
            }
        }

        public virtual bool LinkToGame(IGame game)
        {
            if (game is UnityGame uGame)
            {
                this.game = uGame;
                LinkToGame();
                return true;
            }
            return false;
        }

        public virtual void LinkToGame()
        {
            game.OnStart += GameStart;
            game.OnPause += GamePause;
            game.OnContinue += GameContinue;
            game.OnQuit += GameQuit;

            if(game.GamePhase == GamePhase.Play) GameStart(game);
        }

        public virtual void UnlinkFromGame()
        {
            game.OnStart -= GameStart;
            game.OnPause -= GamePause;
            game.OnContinue -= GameContinue;
            game.OnQuit -= GameQuit;

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
                T tl = default;
                if (gameBoard) { tl = gameBoard.GetLanguage<T>(); }
                if (tl.Equals(default) && game) { tl = game.GetLanguage<T>(); }
                return tl;
            }
        }

        public virtual UnityLanguageAsset GetLanguage()
        {
            if (!languageAsset)
            {
                languageAsset = UnityLanguageAsset.CurrentLanguageAsset;
            }

            if (languageAsset) { return languageAsset; }
            else
            {
                UnityLanguageAsset la = null;
                if (game) {  la = game.GetLanguage(); }
                if(la == null) { la = gameBoard.GetLanguage(); }
                return la;
            }
        }

        #endregion

        #region IAffinity Implementation
        
        public virtual T GetSemanticManager<T>()where T : ISemanticManager
        {
            if(semanticManager is T tManager) { return tManager; }
            return default;
        }

        public virtual Sememe[] GetSememes()
        {
            Sememe[] sememes = semanticManager.GetSememes(GetSememeIDs());

            return sememes;
        }

        public virtual T[] GetSememes<T>() where T : ISememe
        {
            T[] tSememes = new T[properties.Length];
            Sememe[] sememes = semanticManager.GetSememes(GetSememeIDs());
            for (int i = 0; i < properties.Length; i++) 
            {
                if(sememes[i] is T tSememe)
                {
                    tSememes[i] = tSememe;
                }
                else
                {
                    tSememes[i] = default;
                }
            }
            return tSememes;
        }

        public virtual Int[] GetSemanticAffinities()
        {
            Int[] semanticAffinities = new Int[properties.Length];
            for (int i = 0; i < properties.Length; i++)
            {
                semanticAffinities[i] = properties[i].value;
            }
            return semanticAffinities;
        }

        public virtual T[] GetSemanticAffinities<T>() where T : IValue<T>
        {

            if (typeof(T).IsAssignableFrom(typeof(Int)))
            {
                Int[] semanticAffinities = GetSemanticAffinities();

                return semanticAffinities as T[];
            }
            return new T[0];
        }

        public virtual Int[] GetResourceValues()
        {
            Int[] resourceValues = new Int[resources.Length];
            for (int i = 0; i < resources.Length; i++)
            {
                resourceValues[i] = resources[i].value;
            }
            return resourceValues;
        }

        public virtual Int[] GetResourceMaxValues()
        {
            Int[] resourceValues = new Int[resources.Length];
            for (int i = 0; i < resources.Length; i++)
            {
                resourceValues[i] = resources[i].maxValue;
            }
            return resourceValues;
        }

        public virtual int AddSememe(int globalSememeID)
        {
            return AddSemanticAffinity(globalSememeID, 0);
        }

        public virtual int AddResource(int globalSememeID)
        {
            

            int id = GetLocalResourceID(globalSememeID);
            if (id >= 0) { return id; }

            id = resources.Length;
            System.Array.Resize(ref resources, id + 1);

            Debug.Log("Adding Resource #" + globalSememeID);
            
            var res = resources[id];
            res.sememe = globalSememeID;
            resources[id] = res;
            //resources[id].sememe = globalSememeID;

            return id;
        }

        public virtual int AddSemanticAffinity(int globalSememeID, Int affinity)
        {
            int id = GetLocalSememeID(globalSememeID);
            if (id >= 0) { return id; }

            id = properties.Length;
            System.Array.Resize(ref properties, id + 1);
            properties[id].sememe = globalSememeID;
            properties[id].value = affinity;

            return id;
        }

        public virtual int AddSemanticAffinity<T>(int globalSememeID, T affinity)
        {
            int id = GetLocalSememeID(globalSememeID);
            if (id >= 0) { return id; }

            id = properties.Length;
            System.Array.Resize(ref properties, id + 1);
            properties[id].sememe = globalSememeID;
            if(affinity is Int tAff) { properties[id].value = tAff; }

            return id;
        }

        public virtual Int GetSememeAffinity(int localID)
        {
            if (localID < 0 || localID >= properties.Length) { return 0; }
            return properties[localID].value;
        }

        public virtual T GetSememeAffinity<T>(int localID) where T : IValue<T>
        {
            if (localID < 0 || localID >= properties.Length) { return default; }
            if (properties[localID].value is T tSem) { return tSem; }
            return default;
        }


        public virtual int GetSememeID(int localID)
        {
            if(localID >= properties.Length || localID < 0) { return 0; }
            return properties[localID].Sememe();
        }

        public virtual int[] GetSememeIDs()
        {
            int[] sememeIDs = new int[properties.Length];
            for(int i =0; i < properties.Length; i++)
            {
                sememeIDs[i] = properties[i].Sememe();
            }
            return sememeIDs;
        }

        public virtual int[] GetResourceSememeIDs()
        {
            int[] sememeIDs = new int[resources.Length];
            for (int i = 0; i < resources.Length; i++)
            {
                sememeIDs[i] = resources[i].Sememe();
            }
            return sememeIDs;
        }


        public virtual int GetLocalSememeID(int globalSememeID)
        {
            for(int i =0; i < properties.Length; i++)
            {
                if(properties[i].Sememe() == globalSememeID) { return i; }
            }
            return -1;
        }

        public virtual int GetLocalResourceID(int globalSememeID)
        {
            for (int i = 0; i < resources.Length; i++)
            {
                if (resources[i].Sememe() == globalSememeID) { return i; }
            }
            return -1;
        }

        public virtual void SetSememeAtID(int newSememeID, int id)
        {
            if (id >= properties.Length || id < 0) { return; }
            UnityGamePieceProperty prop = properties[id];
            prop.sememe = newSememeID;
            properties[id] = prop;
        }

        public virtual void SetSememeAffinity(int localID, Int newAffinity)
        {
            if (localID < 0 || localID >= properties.Length) { return; }
            UnityGamePieceProperty prop = properties[localID];
            prop.value = newAffinity;
            properties[localID] = prop;
        }

        public virtual void SetSememeAffinity<T>(int localID, T newAffinity) where T : IValue<T>
        {
            if (localID < 0 || localID >= properties.Length) { return; }
            if (newAffinity is Int newInt)
            {
                UnityGamePieceProperty prop = properties[localID];
                prop.value = newInt;
                properties[localID] = prop;
            }
        }


        public virtual void RemoveSememe(int toRemove)
        {
            if (toRemove < 0 || toRemove >= properties.Length) { return; }
            List<UnityGamePieceProperty> newSememes = new List<UnityGamePieceProperty>(properties);

            newSememes.RemoveAt(toRemove);

            properties = newSememes.ToArray();
        }

        public virtual void RemoveResource(int toRemove)
        {
            if (toRemove < 0 || toRemove >= resources.Length) { return; }
            List<UnityGamePieceResource> newSememes = new List<UnityGamePieceResource>(resources);

            newSememes.RemoveAt(toRemove);

            resources = newSememes.ToArray();
        }

        public virtual void CompareSemantics(ISemantic other, out int[] sharedSememes, out int[] othersMissingSememes)
        {
            List<int> shared = new List<int>();
            List<int> notShared = new List<int>();

            int[] sememes = GetSememeIDs();
            int[] oC = other.GetSememeIDs();
            
            for (int i = sememes.Length - 1; i >= 0; i--)
            {
                bool has = false;
                for (int j = oC.Length; j >= 0; j--)
                {
                    if (oC[j] == sememes[i])
                    {
                        has = true; break;
                    }
                }
                if (has)
                {
                    shared.Add(i);
                }
                else
                {
                    notShared.Add(i);
                }
            }
            othersMissingSememes = notShared.ToArray();
            sharedSememes = shared.ToArray();
        }

        public virtual int CompareSemanticAffinity(UnityGamePiece otherPiece)
        {
            int score = 0;

            CompareSemantics(otherPiece, out int[] qualities, out int[] lackingQualities);

            for (int j = lackingQualities.Length - 1; j >= 0; j--)
            {
                score += GetSememeAffinity(lackingQualities[j]).Abs();
            }

            for (int j = qualities.Length - 1; j >= 0; j--)
            {
                int qid = properties[qualities[j]].sememe;
                Int siteScore = otherPiece.GetSememeAffinity(qid).Subtract(
                                GetSememeAffinity(qualities[j]));
                score += siteScore.Abs();
            }

            return score;
        }

        public virtual bool CompareSemanticAffinity<T>(IAffinity other, out T degreeOfDifference)
            where T : IValue<T>
        {
            if (typeof(T) != typeof(Int)) { degreeOfDifference = default; return false; }

            T score = default;
            CompareSemantics(other, out int[] qualities, out int[] lackingQualities);

            for (int j = lackingQualities.Length - 1; j >= 0; j--)
            {
                score = score.Add(GetSememeAffinity<T>(lackingQualities[j]).Abs());
            }

            for (int j = qualities.Length - 1; j >= 0; j--)
            {
                int qid = properties[qualities[j]].sememe;
                T siteScore = other.GetSememeAffinity<T>(qid).Subtract(
                                GetSememeAffinity<T>(qualities[j]));
                score = score.Add(siteScore.Abs());
            }

            degreeOfDifference = score;
            return true;
        }
        #endregion

        #region SubPieces

        public virtual int AddSubPiece(UnityGamePiece newSubPiece)
        {
            int id = GetSubPieceID(newSubPiece);
            if(id < 0)
            {
                id = subPieces.Length;
                System.Array.Resize(ref subPieces, id + 1);
                subPieces[id] = newSubPiece;
            }
            OnAddSubPiece?.Invoke(id, newSubPiece);
            return id;
        }

        public virtual bool RemoveSubPiece(UnityGamePiece subPiece)
        {
            int id = GetSubPieceID(subPiece);
            if(id < 0) { return false; }
            else
            {
                return RemoveSubPiece(id);
            }
        }

        public virtual bool RemoveSubPiece(int subPieceID)
        {
            if(subPieceID <0 || subPieceID >= subPieces.Length) { return false; }
            UnityGamePiece comp = subPieces[subPieceID];
            List<UnityGamePiece> newSubPieces = new List<UnityGamePiece>(subPieces);
            newSubPieces.RemoveAt(subPieceID);
            subPieces = newSubPieces.ToArray();
            OnRemoveSubPiece?.Invoke(subPieceID, comp);
            return true;
        }
        public virtual int GetSubPieceID(UnityGamePiece subPiece)
        {
            for(int i=0; i < subPieces.Length; i++)
            {
                if(subPieces[i] == subPiece) { return i; }
            }
            return -1;
        }
        #endregion
    }

}