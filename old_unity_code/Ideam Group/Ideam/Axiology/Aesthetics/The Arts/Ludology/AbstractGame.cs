using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Axiology.Aesthetics.The_Arts.Ludology
{
    public abstract class AbstractGame : IGame
    {

        public virtual event Action<IGame> OnStart, OnPause, OnContinue, OnQuit;
        
        #region Game State

        //[SerializeField]
        public GamePhase gamePhase;

        public GamePhase GamePhase { get { return gamePhase; } protected set { gamePhase = value; } }

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

        public abstract T GetGamePlayer<T>(int id) where T : IGamePlayer;
        public abstract T[] GetGamePlayers<T>() where T : IGamePlayer;

        public abstract T GetGamePiece<T>(int id) where T : IGamePiece;
        public abstract T[] GetGamePieces<T>() where T : IGamePiece;

        public abstract T GetGameBoard<T>(int id) where T : IGameBoard;
        public abstract T[] GetGameBoards<T>() where T : IGameBoard;

    }
}
