using System;
using System.Collections.Generic;
using System.Text;


namespace Ideam.Metaphysics.Ontology.Existence
{
    public interface IImaginary
    {
        bool SetImagination<T>(T imagination) where T : IImagination;
        T GetImagination<T>() where T : IImagination;
    }
}
