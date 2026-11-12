namespace Ideam.Science.Applied.Architecture
{
    public interface ICeiling 
    {
        public T[] CeilingSurfaces<T>() where T : ISurface;
        public T FloorAbove<T>() where T : IFloor;

    }

}