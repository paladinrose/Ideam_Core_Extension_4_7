using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Axiology.Valuation;
using Ideam.Science.Natural.Physics;
namespace Ideam.Science.Natural.Cosmology
{
    public interface IUniverse
    {
        int GetFieldID<T>(T fundamentalField) where T : IFundamentalField;
        T GetField<T>(int fieldID) where T : IFundamentalField;


    }
}
