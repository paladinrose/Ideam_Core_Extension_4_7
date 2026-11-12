using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Linguistics.Lexicology
{
    public interface ILexicon
    {
        T[] Lexemes<T>(int[] lexemeIDs) where T : ILexeme;
        string ToString(int lexemeID);
    }
}
