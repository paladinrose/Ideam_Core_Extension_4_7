namespace Ideam.Science.Applied.Architecture
{
    public interface IBlueprint 
    {
        //Ok, lessee... Defines the overall bounds that everything mapped will be on. In fact, it's a type of map, which is a type of graph.
        T[] GetFeatures<T>() where T : IFeature;
        T[] GetBuildings<T>() where T : IBuilding;

    }
}