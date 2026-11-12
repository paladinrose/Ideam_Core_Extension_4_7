using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    [CreateAssetMenu(fileName = "New Incident", menuName = "Ideam/Narratology/Incident")]
    public class UnityIncident : UnityNarreme, IIncident
    {
        public IncidentStatus status = IncidentStatus.HasNotHappened;
        public IncidentStatus Status 
        {
            get 
            { 
                if(status != IncidentStatus.HasNotHappened) { return status; }
                
                for(int i=0; i < narremeConditions.Length; i++)
                {
                    if (!narremeConditions[i].CheckCondition())
                    {

                    }
                }
                for(int i = 0; i < semanticConditions.Length; i++)
                {
                    if (!semanticConditions[i].CheckCondition())
                    {

                    }

                }
                causalCondition.CheckCondition();

                if(status == IncidentStatus.HasHappened) { OnHappens?.Invoke(this); }
                else if(status == IncidentStatus.CannotHappen) { OnCannotHappen?.Invoke(this); }

                return status; 
            } 
        }

        public virtual event Action<IIncident> OnHappens, OnCannotHappen;

        public virtual event Action<UnityNarreme> OnAddConditionalNarreme, OnRemoveConditionalNarreme;
        public UnityNarreme[] conditionalNarremes = new UnityNarreme[0];
        public string[] conditionalTitles = new string[0];

        public NarremeCondition[] narremeConditions = new NarremeCondition[0];
        public virtual event Action<NarremeCondition> OnAddNarremeCondition, OnRemoveNarremeCondition;

        public SemanticAffinityCondition[] semanticConditions = new SemanticAffinityCondition[0];
        public virtual event Action<SemanticAffinityCondition> OnAddSemanticCondition, OnRemoveSemanticCondition;

        public CausalCondition causalCondition;

        public virtual int AddConditionalNarreme<T>(T conditionalNarreme) where T : INarreme
        {
            if(conditionalNarreme is UnityNarreme conNar)
            {
                return AddConditionalNarreme(conNar, -1);
            }
            return -1;
        }
        public virtual int AddConditionalNarreme(UnityNarreme conditionalNarreme, int id = -1)
        {

            int entryID = GetConditionalNarremeID(conditionalNarreme);
            if (entryID >= 0) { return entryID; }

            List<UnityNarreme> newConditionalNarremes = new List<UnityNarreme>(conditionalNarremes);
            List<string> newTitles = new List<string>(conditionalTitles);

            if (id < 0 || id >= conditionalNarremes.Length)
            {
                entryID = newConditionalNarremes.Count;
                newConditionalNarremes.Add(conditionalNarreme);
                newTitles.Add("");
            }
            else
            {
                newConditionalNarremes.Insert(id, conditionalNarreme);
                newTitles.Insert(id, "");
                entryID = id;
            }

            conditionalNarremes = newConditionalNarremes.ToArray();
            conditionalTitles = newTitles.ToArray();

            OnAddConditionalNarreme?.Invoke(conditionalNarreme);

            return entryID;
        }

        public virtual int GetConditionalNarremeID(UnityNarreme conditionalNarreme)
        {
            for(int i = conditionalNarremes.Length-1; i >=0; i--)
            {
                if(conditionalNarreme == conditionalNarremes[i]) { return i; }
            }
            return -1;
        }

        public virtual void RemoveConditionalNarreme<T>(T toRemove) where T : INarreme
        {
            if(toRemove is UnityNarreme conditionalNarreme)
            {
                int id = GetConditionalNarremeID(conditionalNarreme);
                if(id >= 0) { RemoveConditionalNarreme(id); }
            }
        }
        public virtual void RemoveConditionalNarreme(int toRemove)
        {
            UnityNarreme conditionalNarreme = conditionalNarremes[toRemove];

            List<NarremeCondition> newNarremeConditions = new List<NarremeCondition>(narremeConditions);
            for(int i = newNarremeConditions.Count-1; i >=0; i--)
            {
                NarremeCondition narremeCondition = newNarremeConditions[i];
                if(narremeCondition.conditionObject == toRemove || narremeCondition.conditionSubject == toRemove)
                {
                    newNarremeConditions.RemoveAt(i);
                    continue;
                }
                if (narremeCondition.conditionObject > toRemove) { narremeCondition.conditionObject--; }
                if (narremeCondition.conditionSubject > toRemove) { narremeCondition.conditionSubject--; }
                newNarremeConditions[i] = narremeCondition;
            }
            narremeConditions = newNarremeConditions.ToArray();
            
            List<SemanticAffinityCondition> newSemanticConditions = new List<SemanticAffinityCondition>(semanticConditions);
            for (int i = newSemanticConditions.Count - 1; i >= 0; i--)
            {
                SemanticAffinityCondition semanticCondition = newSemanticConditions[i];
                if (semanticCondition.conditionObject == toRemove || semanticCondition.conditionSubject == toRemove)
                {
                    newSemanticConditions.RemoveAt(i);
                    continue;
                }
                if (semanticCondition.conditionObject > toRemove) { semanticCondition.conditionObject--; }
                if (semanticCondition.conditionSubject > toRemove) { semanticCondition.conditionSubject--; }
                newSemanticConditions[i] = semanticCondition;
            }
            semanticConditions = newSemanticConditions.ToArray();

            List<int> causes = new List<int>(causalCondition.causes);
            List<bool> hasCaused = new List<bool>(causalCondition.hasCaused);
            for(int i = causes.Count-1; i >=0; i--)
            {
                if(causes[i] == toRemove) { causes.RemoveAt(i); hasCaused.RemoveAt(i); }
                else if(causes[i] > toRemove) { causes[i]--; }
            }
            causalCondition.causes = causes.ToArray();
            causalCondition.hasCaused = hasCaused.ToArray();

            List<UnityNarreme> newConditionalNarremes = new List<UnityNarreme>(conditionalNarremes);
            newConditionalNarremes.RemoveAt(toRemove);

            conditionalNarremes = newConditionalNarremes.ToArray();
            OnRemoveConditionalNarreme?.Invoke(conditionalNarreme);
        }


        public virtual void AttemptToCause<T>(T cause) where T : INarreme
        {
            if(cause is UnityNarreme n)
            {
                int causeID = GetConditionalNarremeID(n);
                if(causeID >=0) { AttemptToCause(causeID); }
            }
        }
        public virtual void AttemptToCause(int causeID)
        {
            if (causeID >= 0)
            {
                for (int i = causalCondition.causes.Length - 1; i >= 0; i--)
                {
                    if (causalCondition.causes[i] == causeID) { causalCondition.hasCaused[i] = true; break; }
                }
            }
        }

        public virtual int AddIncidentCondition<T>(T eventCondition) where T : IIncidentCondition
        {
            if(eventCondition is NarremeCondition narremeCondition)
            {
                return AddNarremeCondition(narremeCondition, -1);
            }
            else if(eventCondition is SemanticAffinityCondition semanticCondition)
            {
                return AddSemanticCondition(semanticCondition, -1);
            }
            return -1;
        }
        public virtual int AddNarremeCondition(NarremeCondition narremeCondition, int id = -1)
        {

            int entryID = GetNarremeConditionID(narremeCondition);
            if (entryID >= 0) { return entryID; }


            List<NarremeCondition> newNarremeConditions = new List<NarremeCondition>(narremeConditions);

            if (id < 0 || id >= narremeConditions.Length)
            {
                entryID = newNarremeConditions.Count;
                newNarremeConditions.Add(narremeCondition);
            }
            else
            {
                newNarremeConditions.Insert(id, narremeCondition);
                entryID = id;
            }

            narremeConditions = newNarremeConditions.ToArray();
            OnAddNarremeCondition?.Invoke(narremeCondition);

            return entryID;
        }
        public virtual int AddSemanticCondition(SemanticAffinityCondition semanticCondition, int id = -1)
        {

            int entryID = GetSemanticConditionID(semanticCondition);
            if (entryID >= 0) { return entryID; }


            List<SemanticAffinityCondition> newSemanticConditions = new List<SemanticAffinityCondition>(semanticConditions);

            if (id < 0 || id >= semanticConditions.Length)
            {
                entryID = newSemanticConditions.Count;
                newSemanticConditions.Add(semanticCondition);
            }
            else
            {
                newSemanticConditions.Insert(id, semanticCondition);
                entryID = id;
            }

            semanticConditions = newSemanticConditions.ToArray();
            OnAddSemanticCondition?.Invoke(semanticCondition);

            return entryID;
        }

        public virtual int GetNarremeConditionID(NarremeCondition condition)
        {
            for (int i = narremeConditions.Length - 1; i >= 0; i--)
            {
                if (narremeConditions[i] == condition) { return i; }
            }
            return -1;
        }
        public virtual int GetSemanticConditionID(SemanticAffinityCondition condition)
        {
            for(int i = semanticConditions.Length-1; i >=0; i--)
            {
                if(semanticConditions[i] == condition) { return i; }
            }
            return -1;
        }
       
        public virtual void RemoveIncidentCondition<T>(T toRemove) where T:IIncidentCondition
        {
            if (toRemove is NarremeCondition narremeCondition)
            {
                int id = GetNarremeConditionID(narremeCondition);
                if (id >= 0) { RemoveNarremeCondition(id); }
            }
            else if (toRemove is SemanticAffinityCondition semanticCondition)
            {
                int id = GetSemanticConditionID(semanticCondition);
                if (id >= 0) { RemoveSemanticCondition(id); }
            }
        }
        public virtual void RemoveNarremeCondition(int toRemove)
        {
            NarremeCondition narremeCondition = narremeConditions[toRemove];
            List<NarremeCondition> newNarremeConditions = new List<NarremeCondition>(narremeConditions);

            newNarremeConditions.RemoveAt(toRemove);

            narremeConditions = newNarremeConditions.ToArray();
            OnRemoveNarremeCondition?.Invoke(narremeCondition);
        }
        public virtual void RemoveSemanticCondition(int toRemove)
        {
            SemanticAffinityCondition semanticCondition = semanticConditions[toRemove];
            List<SemanticAffinityCondition> newSemanticConditions = new List<SemanticAffinityCondition>(semanticConditions);

            newSemanticConditions.RemoveAt(toRemove);

            semanticConditions = newSemanticConditions.ToArray();
            OnRemoveSemanticCondition?.Invoke(semanticCondition);
        }


        #region Narreme Implementation

        public override ConditionStatus CheckNarrativeCondition<T>(int condition, T narremeToCheck = default)
        {
            if (narremeToCheck.Equals(default))
            {
                if (condition == (int)status) { return ConditionStatus.Met; }
            }
            else
            {
                if (condition == 0 && narremeToCheck is UnityNarreme nar)
                {
                    int relID = GetConditionalNarremeID(nar);
                    if (relID >= 0) return ConditionStatus.Met;
                    return ConditionStatus.NotMet;
                }

            }

            return ConditionStatus.NotMet;

        }

        public override T GetLiteralForm<T>()
        {
            int eventID = narrative.GetBeatEvent(narrative.GetBeatID(this));
            if (eventID < 0) return default;
            if (narrative.events[eventID] is T cha) { return cha; }
            return default;
        }

        public override T[] GetSymbolicForms<T>()
        {
            if (!typeof(UnityIncident).IsAssignableFrom(typeof(T)))
            {
                return new T[0];
            }

            int[] symbolicIDs = narrative.GetEventBeats(narrative.GetEventID(this));
            UnityIncident[] beats = new UnityIncident[symbolicIDs.Length];
            for (int i = 0; i < beats.Length; i++)
            {
                beats[i] = narrative.beats[symbolicIDs[i]];
            }
            return beats as T[];
        }

        #endregion

    }

}
