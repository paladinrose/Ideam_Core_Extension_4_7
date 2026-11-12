namespace Ideam.Axiology.Aesthetics.The_Arts.Narratology
{
    #region enums

    /// <summary>
    /// A representation of the state of this plot, based on its events.
    /// </summary>
    public enum PlotStatus { NotBegun, InProgress, Complete, CannotComplete, Failed }

    /// <summary>
    /// A representation of how an events state influences the state of this plot.
    /// </summary>
    public enum PlotIncidentCondition { RequiredHasHappened, RequiredCannotHappen, InstantSuccess, InstantFailure }

    #endregion

    public interface IPlotIncident<T>
        where T:IIncident
    {
        T Incident { get; }
        PlotIncidentCondition Condition { get; }
        string Title { get; }
    }

    public interface IPlot:INarreme
    {
        PlotStatus Status { get; }
        int AddPlotIncident<T, TIncident>(T plotIncident) where T : IPlotIncident<TIncident> where TIncident : IIncident;
        bool RemovePlotIncident<T, TIncident>(T toRemove) where T : IPlotIncident<TIncident> where TIncident : IIncident;
    
    }
}
