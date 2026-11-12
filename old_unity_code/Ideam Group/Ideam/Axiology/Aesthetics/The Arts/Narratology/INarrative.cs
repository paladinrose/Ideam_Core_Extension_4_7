
using System.Text;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Axiology.Valuation;

namespace Ideam.Axiology.Aesthetics.The_Arts.Narratology
{
    public enum ReplaceType
    {
        None, Symbolic, Literal, Thorough
    };

    public enum BestFit
    {
        Basic,
        Symbolic,
        Literal,
        Thorough
    }

    public interface INarrative : INarreme
    {
        
        #region Characters and Roles

        int AddCharacter<T>(T newCharacter) where T : ICharacter;
        void RemoveCharacter<T>(T toRemove) where T : ICharacter;

        int AddRole<T>(T newRole) where T : ICharacter;
        void RemoveRole<T>(T toRemove) where T: ICharacter;

        int GetBestCharacterFitForRole(int roleID, int[] masked, BestFit bestFit = BestFit.Basic);
        bool SetCharacterToRole(int characterID, int roleID, ReplaceType replacement = ReplaceType.None);
        void RemoveCharacterFromRole(int roleID);
        int GetCharacterOfRole(int roleID);
        int[] GetRolesOfCharacter(int characterID);

        #endregion

        #region Locations and Places

        int AddLocation<T>(T newLocation) where T : ILocation;
        void RemoveLocation<T>(T toRemove) where T : ILocation;

        int AddPlace<T>(T newPlace) where T : ILocation;
        void RemovePlace<T>(T toRemove) where T : ILocation;

        int GetBestLocationFitForPlace(int placeID, int[] masked, BestFit bestFit = BestFit.Basic);
        bool SetLocationToPlace(int LocationID, int placeID, ReplaceType replacement = ReplaceType.None);
        void RemoveLocationFromPlace(int placeID);
        int GetLocationInPlace(int placeID);
        int[] GetPlacesForLocation(int locationID);

        #endregion

        #region Objects and Symbols

        int AddObject<T>(T newObject) where T : IProp;
        void RemoveObject<T>(T toRemove) where T : IProp;

        int AddSymbol<T>(T newSymbol) where T : IProp;
        void RemoveSymbol<T>(T toRemove) where T:IProp;

        int GetBestObjectFitForSymbol(int symbolID, int[] masked, BestFit bestFit = BestFit.Basic);
        bool SetObjectToSymbol(int objectID, int symbolID, ReplaceType replacement = ReplaceType.None);
        void RemoveObjectFromSymbol(int symbolID);
        int GetSymbolizedObject(int symbolID);
        int[] GetObjectifiedSymbols(int objectID);

        #endregion

        #region Plots and Stories

        int AddPlot<T>(T newPlot) where T : IPlot;
        void RemovePlot<T>(T toRemove) where T : IPlot;

        int AddStory<T>(T newStory) where T : IPlot;
        void RemoveStory<T>(T toRemove) where T : IPlot;

        int GetBestPlotFitForStory(int storyID, int[] masked, BestFit bestFit = BestFit.Basic);
        bool SetPlotToStory(int plotID, int storyID, ReplaceType replacement = ReplaceType.None);
        void RemovePlotFromStory(int storyID);
        int GetPlotOfStory(int storyID);
        int[] GetStoriesInPlot(int plotID);

        #endregion

        #region Events and Beats

        int AddEvent<T>(T newEvent) where T : IIncident;
        void RemoveEvent<T>(T toRemove) where T : IIncident;

        int AddBeat<T>(T newBeat) where T : IIncident;
        void RemoveBeat<T>(T toRemove) where T : IIncident;

        int GetBestEventFitForBeat(int beatID, int[] masked, BestFit bestFit = BestFit.Basic);
        bool SetEventToBeat(int eventID, int beatID, ReplaceType replacement = ReplaceType.None);
        void RemoveEventFromBeat(int beatID);
        int GetBeatEvent(int beatID);
        int[] GetEventBeats(int eventID);

        #endregion
    }
}
