using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Axiology.Aesthetics.The_Arts.Ludology
{
    public interface IGamePiece
    {
        T GetActiveGame<T>() where T : IGame;

        void GameStart(IGame game);
        void GamePause(IGame game);
        void GameContinue(IGame game);
        void GameQuit(IGame game);
        bool LinkToGame(IGame game);

        public virtual void LinkToGame()
        {
            var game = GetActiveGame<IGame>();
            game.OnStart += GameStart;
            game.OnPause += GamePause;
            game.OnContinue += GameContinue;
            game.OnQuit += GameQuit;

            if (game.GamePhase == GamePhase.Play) GameStart(game);
        }

        public virtual void UnlinkFromGame()
        {
            var game = GetActiveGame<IGame>();
            game.OnStart -= GameStart;
            game.OnPause -= GamePause;
            game.OnContinue -= GameContinue;
            game.OnQuit -= GameQuit;
        }

    }
}
