using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Linguistics.Grammar
{
    public interface ISentence
    {
        T[] GetPhrases<T>() where T : IPhrase;

        int GetPhraseID<T>(T phrase) where T : IPhrase;
        T GetPhrase<T>(int phraseID) where T : IPhrase;

    }
}
