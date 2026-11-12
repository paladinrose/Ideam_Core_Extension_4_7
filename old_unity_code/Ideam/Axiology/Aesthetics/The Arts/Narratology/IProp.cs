namespace Ideam.Axiology.Aesthetics.The_Arts.Narratology
{
    public interface IProp : INarreme
    {
        bool MoveToLocation<T>(T newLocation) where T : ILocation;
        bool PassTo<T>(T character) where T : ICharacter;
        bool CauseEvent<T>(T evnt) where T : IIncident;

        void BecomeLost();

    }
}
