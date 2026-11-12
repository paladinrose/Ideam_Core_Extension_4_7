namespace Ideam.Science.Formal.Graphs
{
    public struct GraphStep
    {
        public int parentID,
                   nodeID, 
                   portID;

        public PortDirection direction;

        public float distanceToStep,
                     heuristicCost;
        public float StepCost { get { return distanceToStep + heuristicCost; } }
    }
}
