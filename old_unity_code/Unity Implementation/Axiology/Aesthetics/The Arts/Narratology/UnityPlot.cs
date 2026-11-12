using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    [CreateAssetMenu(fileName = "New Plot", menuName = "Ideam/Narratology/Plot")]
    public class UnityPlot : UnityNarreme, IPlot
    {
        public PlotStatus status;
        public virtual PlotStatus Status 
        { 
            get 
            {
                if (!(status == PlotStatus.Complete ||
                        status == PlotStatus.Failed))
                {
                    bool allRequired = true, shouldBreak = false;
                    PlotStatus stts = status;
                    for (int i = plotIncidents.Length - 1; i >= 0; i--)
                    {
                        shouldBreak = false;
                        switch (plotIncidents[i].condition)
                        {
                            case PlotIncidentCondition.InstantFailure:
                                if (plotIncidents[i].plotIncident.status == IncidentStatus.HasHappened)
                                {
                                    stts = PlotStatus.Failed;
                                    OnFail?.Invoke(this);
                                    
                                    shouldBreak = true;
                                }

                                break;
                            case PlotIncidentCondition.InstantSuccess:
                                if (plotIncidents[i].plotIncident.status == IncidentStatus.HasHappened)
                                {
                                    stts = PlotStatus.Complete;
                                    OnComplete?.Invoke(this);
                                    
                                    shouldBreak = true;
                                }
                                break;
                            case PlotIncidentCondition.RequiredHasHappened:
                                if (plotIncidents[i].plotIncident.status == IncidentStatus.HasHappened)
                                {
                                    if (stts != PlotStatus.CannotComplete) { stts = PlotStatus.InProgress; }
                                }
                                else if (plotIncidents[i].plotIncident.status == IncidentStatus.CannotHappen)
                                {
                                    stts = PlotStatus.CannotComplete;
                                    OnCannotComplete?.Invoke(this);
                                    allRequired = false;

                                }
                                else
                                {
                                    allRequired = false;
                                }
                                break;

                            case PlotIncidentCondition.RequiredCannotHappen:
                                if (plotIncidents[i].plotIncident.status == IncidentStatus.HasHappened)
                                {
                                    stts = PlotStatus.CannotComplete;
                                    OnCannotComplete?.Invoke(this);
                                    
                                    allRequired = false;
                                }

                                else if (plotIncidents[i].plotIncident.status == IncidentStatus.CannotHappen)
                                {
                                    if (stts != PlotStatus.CannotComplete) { stts = PlotStatus.InProgress; }
                                }
                                else
                                {
                                    allRequired = false;
                                }
                                break;
                        }
                        if (shouldBreak) { break; }
                    }

                    if (stts == PlotStatus.InProgress)
                    {
                        if (status != PlotStatus.CannotComplete)
                        {
                            if (status == PlotStatus.NotBegun)
                            {
                                status = stts;
                                OnBegin?.Invoke(this);
                            }
                            status = stts;
                            if (!shouldBreak && allRequired)
                            {
                                status = PlotStatus.Complete;
                                OnComplete?.Invoke(this);
                                
                            }
                        }
                    }
                    else
                    {
                        status = stts;
                    }
                }

                return status;
            } }

        public virtual event Action<UnityPlot> OnBegin, OnComplete, OnCannotComplete, OnFail;

        public UnityPlotIncident[] plotIncidents;
        public virtual event Action<UnityPlotIncident> OnAddPlotIncident, OnRemovePlotIncident;

        public virtual int AddPlotIncident<T, TEvent>(T plotIncident) where T:IPlotIncident<TEvent> where TEvent : IIncident
        {
            if(plotIncident is UnityPlotIncident newPlotEvent)
            {
                return AddPlotIncident(newPlotEvent, -1);
            }
            return -1;
        }
        public virtual int AddPlotIncident(UnityPlotIncident plotIncident, int id = -1)
        {
            int entryID = GetPlotIncidentID(plotIncident);
            if (entryID >= 0) { return entryID; }

            List<UnityPlotIncident> newPlotIncidents = new List<UnityPlotIncident>(plotIncidents);

            if (id < 0 || id >= plotIncidents.Length)
            {
                entryID = newPlotIncidents.Count;
                newPlotIncidents.Add(plotIncident);
            }
            else
            {
                newPlotIncidents.Insert(id, plotIncident);
                entryID = id;
            }

            plotIncidents = newPlotIncidents.ToArray();
            OnAddPlotIncident?.Invoke(plotIncident);

            return entryID;
        }

        public virtual int GetPlotIncidentID(UnityPlotIncident plotIncident)
        {
            for(int i = plotIncidents.Length-1; i >=0; i--)
            {
                if(plotIncidents[i] == plotIncident) { return i; }
            }
            return -1;
        }
        public virtual int GetPlotIncidentID(UnityIncident incident)
        {
            for(int i = 0; i < plotIncidents.Length-1; i++)
            {
                if(plotIncidents[i].Incident == incident) { return i; }
            }
            return -1;
        }
        public virtual bool RemovePlotIncident<T, TIncident>(T toRemove) where T:IPlotIncident<TIncident> where TIncident:IIncident
        {
            if(toRemove is UnityPlotIncident plotIncident)
            {
                int id = GetPlotIncidentID(plotIncident);
                if(id >= 0)
                {
                    RemovePlotIncident(id);
                    return true;
                }
            }
            return false;
        }
        public virtual void RemovePlotIncident(int toRemove)
        {
            UnityPlotIncident plotIncident = plotIncidents[toRemove];
            List<UnityPlotIncident> newPlotIncidents = new List<UnityPlotIncident>(plotIncidents);

            newPlotIncidents.RemoveAt(toRemove);

            plotIncidents = newPlotIncidents.ToArray();
            OnRemovePlotIncident?.Invoke(plotIncident);
        }


        #region Narreme Implementation

        public override ConditionStatus CheckNarrativeCondition<T>(int condition, T narremeToCheck = default)
        {
            if (narremeToCheck.Equals(default))
            {
                if(condition == (int)Status) { return ConditionStatus.Met; }
            }
            else
            {
                if (narremeToCheck is UnityIncident evnt)
                {
                    switch (condition)
                    {
                        case 0:
                            if (GetPlotIncidentID(evnt) >=0) return ConditionStatus.Met;
                            return ConditionStatus.NotMet;
                    }
                }
                else if (narremeToCheck is UnityPlot plot)
                {
                    switch (condition)
                    {
                        case 0:
                            if(plot == this) { return ConditionStatus.Met; }
                            return ConditionStatus.NotMet;
                    }
                }
            }

            return ConditionStatus.NotMet;

        }

        public override T GetLiteralForm<T>()
        {
            int plotID = narrative.GetPlotOfStory(narrative.GetStoryID(this));
            if (plotID < 0) return default;
            if (narrative.plots[plotID] is T cha) { return cha; }
            return default;
        }

        public override T[] GetSymbolicForms<T>()
        {
            if (!typeof(UnityPlot).IsAssignableFrom(typeof(T)))
            {
                return new T[0];
            }

            int[] symbolicIDs = narrative.GetStoriesInPlot(narrative.GetPlotID(this));
            UnityPlot[] stories = new UnityPlot[symbolicIDs.Length];
            for (int i = 0; i < stories.Length; i++)
            {
                stories[i] = narrative.stories[symbolicIDs[i]];
            }
            return stories as T[];
        }

        #endregion
    }

}
