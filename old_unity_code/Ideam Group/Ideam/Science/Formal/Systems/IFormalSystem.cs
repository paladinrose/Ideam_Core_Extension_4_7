using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Science.Formal.Linguistics;

namespace Ideam.Science.Formal.Systems
{
    public interface IFormalSystem : ISystem
    {
        //T[] GetFormalSymbols<T>() where T : ISymbol;
        T GetFormalLanguage<T>() where T : ILanguage;

    }
}
