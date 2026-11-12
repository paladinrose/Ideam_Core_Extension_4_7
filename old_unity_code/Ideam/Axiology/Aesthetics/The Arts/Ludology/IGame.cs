using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Axiology.Aesthetics.The_Arts.Ludology
{
    
    public interface IGame
    {
        event Action<IGame> OnStart, OnPause, OnContinue, OnQuit;

        GamePhase GamePhase { get; }

        void StartGame();
        IEnumerator GameSetup();
        void PauseGame();
        void ContinueGame();
        void QuitGame();

        T GetGamePlayer<T>(int id) where T : IGamePlayer;
        T[] GetGamePlayers<T>() where T : IGamePlayer;

        T GetGamePiece<T>(int id) where T : IGamePiece;
        T[] GetGamePieces<T>() where T : IGamePiece;

        T GetGameBoard<T>(int id) where T : IGameBoard;
        T[] GetGameBoards<T>() where T : IGameBoard;
    }


}
