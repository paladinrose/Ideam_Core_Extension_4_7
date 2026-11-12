using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Linguistics.Grammar
{
    public interface IGrammar
    {
        T[] GetSyntaxes<T>() where T : ISyntax;

        T GetSentence<T>(int[] sememes) where T : ISentence;
        T GetPhrase<T>(int[] sememes) where T : IPhrase;
    }
}
