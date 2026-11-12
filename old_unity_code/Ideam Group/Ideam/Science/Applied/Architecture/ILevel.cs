namespace Ideam.Science.Applied.Architecture
{
    public interface ILevel 
    {
        T[] GetRooms<T>() where T : IRoom;
    }
}
