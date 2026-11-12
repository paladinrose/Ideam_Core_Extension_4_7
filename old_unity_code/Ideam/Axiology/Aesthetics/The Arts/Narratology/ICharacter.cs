using System.Text;
namespace Ideam.Axiology.Aesthetics.The_Arts.Narratology
{
    public interface ICharacter : INarreme
    {
        #region Events
        bool CauseEvent<T, TCharacter, TObject>(T evnt, TCharacter[] partners, TObject[] usedObjects) 
            where T : IIncident where TCharacter : ICharacter where TObject : IProp;

        #endregion

            #region Location
        bool MoveToLocation<T>(T newLocation) where T : ILocation;
        #endregion

        #region Goals

        int AddGoal<T>(T newGoal) where T : IPlot;
        void RemoveGoal<T>(T toRemove) where T : IPlot;

        #endregion

        #region Possessions

        int TakePossession<T>(T newPossession) where T : IProp;
        void LosePossession<T>(T toRemove) where T : IProp;

        #endregion

        #region Relationships

        int BeginRelationship<T>(T subject) where T : INarreme;
        void EndRelationship<T>(T subject) where T : INarreme;

        #endregion
    }

}
