using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Metaphysics.Ontology.Action
{
    public interface IInteraction
    {
        T[] CausalAgents<T>() where T : IAgent;
        T[] CausalPieces<T>() where T : IThing;
        T[] CausalActions<T>() where T : IAction;

        T[] EffectedAgents<T>() where T : IAgent;
        T[] EffectedPieces<T>() where T : IThing;
        
        void BeginInteraction();
        void EndInteraction();
        T[] MotivateReactions<T>() where T:IAction;
    }
}
