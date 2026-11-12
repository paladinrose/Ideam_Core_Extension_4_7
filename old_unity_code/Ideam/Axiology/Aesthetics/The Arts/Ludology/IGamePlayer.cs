using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Metaphysics.Ontology.Action;
namespace Ideam.Axiology.Aesthetics.The_Arts.Ludology
{
    public interface IGamePlayer
    {
        event Action<IAgent> OnTakeControlOfAgent, OnReleaseAgent;

        T[] GetPlayerControlledPieces<T>() where T : IGamePiece;
        T[] GetControlledAgents<T>() where T : IAgent;
        int TakeControlOfAgent<T>(T agent) where T : IAgent;
        bool ReleaseAgent(int id);
    }


}
