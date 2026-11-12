using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Science.Natural.Chemistry
{
    public interface IMolecule
    {
        T[] GetAtoms<T>() where T : IAtom;
    }
}
