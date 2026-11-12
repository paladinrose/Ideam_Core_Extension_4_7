using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Axiology.Aesthetics.The_Arts.Narratology
{
    public interface ILocation : INarreme, IPlace
    {
        int EnterCharacter<T>(T character) where T : ICharacter;
        void ExitCharacter<T>(T character) where T : ICharacter;
        int GetProp<T>(T obj) where T : IProp;
        void LoseProp<T>(T obj) where T : IProp;

    }
}
