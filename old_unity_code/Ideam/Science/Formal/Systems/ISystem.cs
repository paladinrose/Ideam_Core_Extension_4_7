using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Science.Formal.Mathematics.Information.Cryptography;

namespace Ideam.Science.Formal.Systems
{
    public interface ISystem
    {
        T GetEnvironment<T>() where T : IPlace;

        T GetUser<T>(int id) where T : IUser;
        T[] GetUsers<T>() where T : IUser;

        //Principles of System Theory to apply:

        /*
        System: a group of interacting, interdependent parts that form a complex whole.[3]
        Boundaries: barriers that define a system and distinguish it from other systems in an environment.
        Homeostasis: the tendency of a system to be resilient with respect to external disruption and to maintain its key characteristics.
        Adaptation: the tendency of a system to make the internal changes to protect itself and keep fulfilling its purpose.
        Reciprocal transactions: circular or cyclical interactions that systems engage in such that they influence one another.
        Feedback loop: the process by which systems self-correct based on reactions from other systems in the environment.
        Throughput: the rate of energy transfer between a system and its environment over time.
        Microsystem: the system closest to the client.
        Mesosystem: relationships among systems in an environment.
        Exosystem: a relationship between two systems that has an indirect effect on a third system.
        Macrosystem: a larger system that influences clients, such as policies, administration of entitlement programs, and culture.
        Equifinality: the way systems can reach the same goal through different paths.[3]
        Open and closed systems[3]
        Chronosystem: a system composed of significant life events affecting adaptation.
        Isomorphism: structural, behavioral, and developmental features that are shared across systems.[3]
        Systems architecture:
        Systems analysis:

        */

        T Security<T>() where T : ISecurity;
    }
}