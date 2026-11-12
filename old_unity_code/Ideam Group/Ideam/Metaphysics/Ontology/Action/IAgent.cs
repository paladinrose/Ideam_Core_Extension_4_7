using System;
using System.Collections.Generic;
using System.Text;

using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Metaphysics.Mind;
namespace Ideam.Metaphysics.Ontology.Action
{
    public interface IAgent
    {
        event Action<IAction> OnActionStart, OnActionUpdate, OnActionComplete, OnActionFail, OnActionInterrupted;

        T GetMind<T>() where T : IMind;
        T[] GamePieces<T>() where T : IThing;
        T[] Actions<T>() where T : IAction;
        T[] InProgressActions<T>() where T : IAction;

        void StartAction(int actionID);
        void UpdateAction(int actionID);
        void InterruptAction(int actionID);
        void CompleteAction(int actionID);
        void FailAction(int actionID);
    }
}
