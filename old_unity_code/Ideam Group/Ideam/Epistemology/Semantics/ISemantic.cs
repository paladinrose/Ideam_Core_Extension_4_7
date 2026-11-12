using System;
using System.Collections.Generic;
using System.Text;

namespace Ideam.Epistemology.Semantics
{
    //BIG NOTE TO SELF: SememeID 0 == NOTHING. ZERO. NADA. ZIP. ABSENCE. VOID.
    //Negative values are totally fine to use.
    public interface ISemantic
    {
        T GetSemanticManager<T>() where T : ISemanticManager;
        T[] GetSememes<T>() where T:ISememe;
        int AddSememe(int globalSememeID);
        int GetSememeID(int localID);
        int[] GetSememeIDs();
        int GetLocalSememeID(int globalSememeID);
        void SetSememeAtID(int newSememeID, int id);
        void RemoveSememe(int toRemove);
        void CompareSemantics(ISemantic other, out int[] sharedSememes, out int[] missingSememes);
    }
}
