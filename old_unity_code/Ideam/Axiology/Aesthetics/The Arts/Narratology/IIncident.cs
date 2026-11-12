using System;

namespace Ideam.Axiology.Aesthetics.The_Arts.Narratology
{
    #region Enums
    public enum IncidentStatus
    {
        HasNotHappened, HasHappened, CannotHappen
    }
    #endregion

    public interface IIncident : INarreme
    {
        event Action<IIncident> OnHappens, OnCannotHappen;
        IncidentStatus Status { get; }

        int AddConditionalNarreme<T>(T conditionalNarreme) where T : INarreme;
        void RemoveConditionalNarreme<T>(T conditionalNarreme) where T : INarreme;

        int AddIncidentCondition<T>(T incidentCondition) where T : IIncidentCondition;
        void RemoveIncidentCondition<T>(T toRemove) where T:IIncidentCondition;

        void AttemptToCause<T>(T cause) where T : INarreme;
    }

    public interface IIncidentCondition 
    {
        bool CheckCondition();
    }
}
