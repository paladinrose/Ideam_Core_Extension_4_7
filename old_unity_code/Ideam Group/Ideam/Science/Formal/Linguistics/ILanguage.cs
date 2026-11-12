using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Science.Formal.Linguistics.Grammar;
using Ideam.Science.Formal.Linguistics.Lexicology;

namespace Ideam.Science.Formal.Linguistics
{
    public interface ILanguage
    {
        T GetLects<T>() where T : ILect;
        T GetLexicons<T>() where T : ILexicon;
        T GetGrammars<T>() where T : IGrammar;

        T GetSentence<T>(int[] sememes) where T : ISentence;
        T GetPhrase<T>(int[] sememes) where T : IPhrase;
    }


}
