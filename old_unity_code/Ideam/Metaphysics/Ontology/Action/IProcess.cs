using System;

using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Metaphysics.Ontology.Action
{
    public interface IProcess
    {
        int ProcessLength { get; }
        int[] ProcessActionIDs { get; }

        T GetProcessor<T>(int id) where T : IThing;
        T[] GetProcessors<T>() where T : IThing;
        
        void StartProcessing();
        void Process();
        void StopProcessing();
        void ProcessComplete();
    }

    public abstract class AbstractProcess :IProcess
    {
        public virtual event System.Action OnStartProcessing, OnStopProcessing, OnProcessComplete;
        public virtual event Action<int> OnProcess;
        
        protected int currentProcessAction;
        protected int[] processActionIDs;

        public virtual int ProcessLength { get { return processActionIDs.Length; } }
        public virtual int[] ProcessActionIDs { get { return processActionIDs; } }

        public abstract T GetProcessor<T>(int id) where T : IThing;
        public abstract T[] GetProcessors<T>() where T : IThing;

        

        public virtual void StartProcessing()
        {
            OnStartProcessing?.Invoke();
        }

        public virtual void Process()
        {
            if (currentProcessAction < ProcessLength)
            {
                IThing stepPart = GetProcessor<IThing>(currentProcessAction);
                OnProcess?.Invoke(currentProcessAction);
                currentProcessAction++;
            }
            else if (currentProcessAction == ProcessLength) { ProcessComplete(); }
        }

        public virtual void StopProcessing()
        {
            OnStopProcessing?.Invoke();
        }

        public virtual void ProcessComplete()
        {
            OnProcessComplete?.Invoke();
        }
    }
}
