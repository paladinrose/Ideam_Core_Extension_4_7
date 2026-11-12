using Ideam.Science.Formal.Mathematics;

namespace Ideam.Science.Applied.Architecture
{
    public interface ISurface
    {

        T[] Apertures<T>() where T : IAperture;

        T GetAperture<T>(int id) where T:IAperture;

        void ApertureUpdated<T>(T aperture);
    }

}