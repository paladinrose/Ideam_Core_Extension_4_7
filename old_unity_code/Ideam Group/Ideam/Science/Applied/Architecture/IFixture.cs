using Ideam.Science.Formal.Mathematics;

namespace Ideam.Science.Applied.Architecture
{
    public interface IFixture : IFeature
    {
        Float2 Position { get; }
        T Surface<T>() where T : ISurface;
    }
}