using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Science.Natural.Physics;

namespace Ideam.Science.Natural.Chemistry
{
    public interface IAtom
    {
        T[] GetParticles<T>() where T : IQuanta;

    }
}
