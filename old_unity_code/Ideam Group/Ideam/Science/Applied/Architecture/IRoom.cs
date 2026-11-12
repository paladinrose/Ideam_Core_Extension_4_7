namespace Ideam.Science.Applied.Architecture
{
    public interface IRoom
    {
        public T[] Walls<T>() where T : IWall;

        public T[] Floors<T>() where T : IFloor;

        public T[] Ceilings<T>() where T : ICeiling;

        public T[] Surfaces<T>() where T : ISurface;
    }
}