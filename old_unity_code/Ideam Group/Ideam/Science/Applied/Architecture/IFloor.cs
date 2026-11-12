using Ideam.Science.Formal.Mathematics;

namespace Ideam.Science.Applied.Architecture
{
    
    public interface IFloor 
    {
        public T[] FloorSurfaces<T>() where T : ISurface;
        public T CeilingBelow<T>() where T : ICeiling;
    }

}