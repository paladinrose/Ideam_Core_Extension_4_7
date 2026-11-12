using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Axiology.Valuation;
using Ideam.Axiology.Aesthetics.The_Arts.Ludology;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Natural.Physics.QuantumMechanics;
using Ideam.Science.Natural.Physics;
using Ideam.Science.Formal.Linguistics;
using Ideam.Science.Natural.Cosmology;


using Ideam.Unity.Science.Applied.Audio;
using Ideam.Unity.Science.Natural.Physics;
using Ideam.Unity.Science.Natural.Cosmology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class UnityGameBoard : MonoBehaviour, IGameBoard, IPlace, IReal, IHilbertSpace, ILanguageProvider
    {
        public MusicZone[] musicZones;
        public Canvas[] canvases;
        public UnityGame game;
        public UnityGamePiece[] gamePieces;
        public UnityGameAgent toPossessOnAwake;
        
        
        public UnityLanguageAsset languageAsset;

        public virtual void BoardAwake()
        {
            //This is gonna Error until I make sure LocalUser and Player both exist. Which is good. A good reminder.
            if (toPossessOnAwake) { game.Cosmos.LocalUser.Player.TakeControlOfAgent(toPossessOnAwake); }

            for(int i =0; i < canvases.Length; i++)
            {
                canvases[i].worldCamera = game.Cosmos.cosmicCamera;
            }

            for(int i =0; i < musicZones.Length; i++)
            {
                musicZones[i].GlobalJukebox = game.Cosmos.jukebox;
            }
        }

        #region IPlace Implementation

        public virtual T[] GetThingsWithin<T>() where T : IThing
        {
            List<T> tGamePieces = new List<T>();
            for (int i = 0; i < gamePieces.Length; i++)
            {
                if (gamePieces[i] is T tGamePiece)
                {
                    tGamePieces.Add(tGamePiece);
                }
            }
            return tGamePieces.ToArray();
        }

        #endregion

        #region IReal Implementation

        public virtual bool SetReality<T>(T reality) where T : IReality
        {
            if (reality is UnityGame realGame)
            {
                game = realGame;
            }
            return false;
        }

        public virtual T GetReality<T>() where T : IReality
        {
            if (game is T tGame) return tGame;

            return default;
        }

        #endregion

        #region IGameBoard Implementation

        public T[] GetGamePieces<T>() where T : IGamePiece
        {
            List<T> tGamePieces = new List<T>();
            for (int i = 0; i < gamePieces.Length; i++)
            {
                if (gamePieces[i] is T tGamePiece)
                {
                    tGamePieces.Add(tGamePiece);
                }
            }
            return tGamePieces.ToArray();
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
                if (game) { tl = game.GetLanguage<T>(); }
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
            else if( game){ return game.GetLanguage(); }

            return null;
        }

        #endregion

        #region IHilbertSpace Implementation

        public virtual T Bounds<T, TValue>() where T : IBoundary<T, TValue> where TValue : IValue<TValue>
        {
            return default;
        }

        public virtual T Universe<T>() where T : IUniverse
        {
            if(game is T tGame) { return tGame; }
            return default;
        }
        
        public virtual T GetFieldLimit<T>(int id) where T : IValue<T>
        {
            throw new NotImplementedException();
        }

        public virtual T Sample<T, TID>(TID location, int fieldID)
            where T : IQuanta
            where TID : IValue<TID>
        {
            throw new NotImplementedException();
        }

        #endregion
    }

    public class UnityGameBoardSpace : IQuanta
    {
        public bool IsFundamental { get; }
        public Int[] intValues;
        public Float[] floatValues;

        public T[] ActualValues<T>() where T : IValue<T>
        {
            //If they request ints, give them int values.
            //If they request int2's, set each intValue up as a pair, 1&2, 3&4, etc.
            //Do the same with floats.
            return default;
        }

        public T Measure<T, TField, TPointer>(TField field, TPointer pointerState)
            where T : IValue<T>
            where TField : IFundamentalField
            where TPointer : IPointerState
        {
            if(field is UnityIntegerField intField)
            {
                
            }
            return default;
        }
    }
}