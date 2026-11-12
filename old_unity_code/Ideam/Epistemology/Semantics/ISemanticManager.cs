using System.Collections.Generic;
using System.Text;

namespace Ideam.Epistemology.Semantics
{
    //BIG NOTE TO SELF: SememeID 0 == NOTHING. ZERO. NADA. ZIP. ABSENCE. VOID.
    //Negative values are totally fine to use.
    public interface ISemanticManager
    {
        int GetSememeID<T>(T sememe) where T : ISememe;
        T GetSememe<T>(int sememeID) where T : ISememe;
        T[] GetSememes<T>(int[] sememeIDs) where T : ISememe;
        int[] GetDenotation(int sememeID);
        int[] GetConnotation(int sememeID, int[] context);
    }
}
