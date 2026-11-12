using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Natural.Chemistry
{
    public interface IChemical
    {
        T[] GetMolecules<T>() where T : IMolecule;
    }
}
