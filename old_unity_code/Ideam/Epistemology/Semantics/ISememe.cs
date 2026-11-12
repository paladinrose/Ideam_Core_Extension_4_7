using System.Collections.Generic;
using System.Text;

namespace Ideam.Epistemology.Semantics
{
    public interface ISememe
    {
        SememicForm Form { get; }

        int GetDimensionID (int dimensionID);
        int GetDomainID (int domainID);
        int GetTaxemeID (int taxemeID);

        int GetSemeID (int semeID);
        bool IsActualized (int semeID, bool isLocal = true);
        int[] GetSemanteme ();

        

    }
}
