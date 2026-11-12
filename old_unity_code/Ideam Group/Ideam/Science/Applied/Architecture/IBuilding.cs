namespace Ideam.Science.Applied.Architecture
{ 
    public interface IBuilding : IStructure
    {
        
        T[] GetAllLevels<T>() where T : ILevel;
        T[] GetAboveGroundLevels<T>() where T : ILevel;
        T[] GetBasementLevels<T>() where T : IBasement;
        T[] GetAtticLevels<T>() where T : IAttic;
    }
}