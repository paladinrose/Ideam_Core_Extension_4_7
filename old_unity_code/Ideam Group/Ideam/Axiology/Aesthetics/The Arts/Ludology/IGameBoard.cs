using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Axiology.Aesthetics.The_Arts.Ludology
{
    public interface IGameBoard
    {
        T[] GetGamePieces<T>() where T : IGamePiece;


    }
}
