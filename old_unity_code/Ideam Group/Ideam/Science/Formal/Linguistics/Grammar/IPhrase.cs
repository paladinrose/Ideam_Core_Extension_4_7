using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Formal.Linguistics.Grammar
{
    public interface IPhrase
    {
        void GetLexemes(out int[] lexemeIDs, out int[] inflections);
        int GetLexeme(int id);
        int GetInflection(int id);
    }
}
