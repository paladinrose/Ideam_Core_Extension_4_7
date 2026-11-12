using System;
using System.Collections.Generic;
using System.Text;


namespace Ideam.Metaphysics.Ontology.Existence
{
    public interface IReal
    {
        bool SetReality<T>(T reality) where T : IReality;
        T GetReality<T>() where T : IReality;
    }
}
