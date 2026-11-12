using System;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{

    [Serializable]
    public class UnityPlotIncident : IPlotIncident<UnityIncident>
    {
        public UnityIncident plotIncident;
        public UnityIncident Incident { get { return plotIncident; } }
        
        public string title = string.Empty;
        public string Title { get { return title; } }

        public PlotIncidentCondition condition = PlotIncidentCondition.RequiredHasHappened;
        public PlotIncidentCondition Condition { get { return condition; } }

        public static int Contains(UnityPlotIncident[] events, UnityIncident evnt)
        {
            for (int i = events.Length - 1; i >= 0; i--)
            {
                if (events[i].plotIncident == evnt) { return i; }
            }
            return -1;
        }
    }
}
