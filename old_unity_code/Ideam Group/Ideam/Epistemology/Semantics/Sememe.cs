using System;

namespace Ideam.Epistemology.Semantics
{
    [Serializable]
    public class Sememe : ISememe
    {
        public int uniqueID;

        public SememicForm Form { get; private set; }
        public int[] semes;
        public Sememe()
        {
            Form = SememicForm.NotSet;
        }
        public Sememe(SememicForm form)
        {
            Form = form;
        }
        public void SetForm(SememicForm form)
        {
            if(Form != SememicForm.NotSet) { return; }
            Form = form;
        }
        public int GetDimensionID(int dimensionID)
        {
            throw new NotImplementedException();
        }

        public int GetDomainID(int domainID)
        {
            throw new NotImplementedException();
        }

        public int[] GetSemanteme()
        {
            throw new NotImplementedException();
        }

        public int GetSemeID(int semeID)
        {
            throw new NotImplementedException();
        }

        public int GetTaxemeID(int taxemeID)
        {
            throw new NotImplementedException();
        }

        public bool IsActualized(int semeID, bool isLocal = true)
        {
            throw new NotImplementedException();
        }
    }
}
