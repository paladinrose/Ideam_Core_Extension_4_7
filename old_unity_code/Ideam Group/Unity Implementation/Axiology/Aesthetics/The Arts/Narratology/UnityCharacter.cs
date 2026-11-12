using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    [CreateAssetMenu(fileName = "New Character", menuName = "Ideam/Narratology/Character")]
    public class UnityCharacter : UnityNarreme, ICharacter
    {
        #region Events

        public UnityIncident[] causedEvents = new UnityIncident[0];

        public virtual event Action<UnityIncident> OnCauseEvent;

        public virtual bool CauseEvent<T, TCharacter, TProp>(T evnt, TCharacter[] partners, TProp[] usedProps) 
            where T : IIncident
            where TCharacter: ICharacter
            where TProp : IProp
        {
            if (evnt is UnityIncident eventual)
            {
                if (eventual.Status == IncidentStatus.HasHappened)
                {
                    OnCauseEvent?.Invoke(eventual);
                    return true;
                }
            }
            return false;
        }

        #endregion

        #region Location

        public UnityLocation currentLocation;

        public virtual event Action<UnityLocation> OnEnterLocation, OnLeaveLocation;

        public virtual bool MoveToLocation<T>(T newLocation) where T : ILocation
        {
            if(newLocation is UnityLocation loc)
            {
                if (currentLocation != null)
                {
                    currentLocation.ExitCharacter(this);
                    OnLeaveLocation?.Invoke(currentLocation);

                }

                currentLocation = loc;

                loc.EnterCharacter(this);
                OnEnterLocation?.Invoke(loc);
            }
            return false;
        }

        #endregion

        #region Goals
        
        public UnityPlot[] goals = new UnityPlot[0];
        
        public virtual event Action<UnityPlot> OnAddGoal, OnRemoveGoal;

        public virtual int AddGoal<T>(T newGoal) where T : IPlot
        {
            if (newGoal is UnityPlot goal)
            {
                return AddGoal(goal, -1);
            }
            return -1;
        }

        public virtual int AddGoal(UnityPlot goal, int id = -1)
        {
            int entryID = GetGoalID(goal);
            if (entryID >= 0) { return entryID; }


            List<UnityPlot> newGoals = new List<UnityPlot>(goals);

            if (id < 0 || id >= goals.Length)
            {
                entryID = newGoals.Count;
                newGoals.Add(goal);
            }
            else
            {
                newGoals.Insert(id, goal);
                entryID = id;
            }

            goals = newGoals.ToArray();
            OnAddGoal?.Invoke(goal);

            return entryID;
        }

        public virtual int GetGoalID(UnityPlot goal)
        {
            for(int i = goals.Length-1; i >=0; i--)
            {
                if(goals[i] == goal) { return i; }
            }
            return -1;
        }

        public virtual void RemoveGoal<T>(T toRemove) where T : IPlot
        {
            if(toRemove is UnityPlot goal)
            {
                int id = GetGoalID(goal);
                if (id >= 0) RemoveGoal(id);
            }
        }

        public virtual void RemoveGoal(int toRemove)
        {
            UnityPlot goal = goals[toRemove];
            List<UnityPlot> newGoals = new List<UnityPlot>(goals);

            newGoals.RemoveAt(toRemove);

            goals = newGoals.ToArray();
            OnRemoveGoal?.Invoke(goal);
        }

        #endregion

        #region Possessions
        
        public UnityProp[] possessions = new UnityProp[0];
        
        public virtual event Action<UnityProp> OnTakePossession, OnLosePossession;

        public virtual int GetPossessionID(UnityProp possession) 
        {
            for(int i = possessions.Length-1; i >=0; i--)
            {
                if(possessions[i] == possession) { return i; }
            }
            return -1;
        }

        public virtual int TakePossession<T>(T newPossession) where T : IProp
        {
            if (newPossession is UnityProp newProp)
            {
                return TakePossession(newProp, -1);
            }
            return -1;
        }

        public virtual int TakePossession(UnityProp possession, int id = -1)
        {
            int entryID = GetPossessionID(possession);
            if (entryID >= 0) { return entryID; }

            List<UnityProp> newPossessions = new List<UnityProp>(possessions);

            if (id < 0 || id >= possessions.Length)
            {
                entryID = newPossessions.Count;
                newPossessions.Add(possession);
            }
            else
            {
                newPossessions.Insert(id, possession);
                entryID = id;
            }

            possessions = newPossessions.ToArray();
            if (possession.currentPossessor != null)
            {
                possession.currentPossessor.LosePossession(possession);
            }


            possession.currentPossessor = this;

            OnTakePossession?.Invoke(possession);

            return entryID;
        }

        public virtual void LosePossession<T>(T lostPossession) where T : IProp
        {
            if (lostPossession is UnityProp lost)
            {
                int id = GetPossessionID(lost);
                if (id >= 0) { LosePossession(id); }
            }
        }

        public virtual void LosePossession(int toRemove)
        {
            UnityProp losing = possessions[toRemove];
            List<UnityProp> newPossessions = new List<UnityProp>(possessions);
            newPossessions.RemoveAt(toRemove);
            possessions = newPossessions.ToArray();
            OnLosePossession?.Invoke(losing);
        }

        #endregion

        #region Relationships

        public UnityRelation[] relationships = new UnityRelation[0];
        public virtual event Action<UnityNarreme> OnBeginRelationship, OnEndRelationship;

        public virtual int BeginRelationship<T>(T subject) where T : INarreme
        {
            if(subject is UnityNarreme nar)
            {
                return BeginRelationship(nar, -1);
            }
            return -1;
        }

        public virtual int BeginRelationship(UnityNarreme subject, int id = -1)
        {
            int entryID = GetRelationshipID(subject);
            if (entryID >= 0) { return entryID; }


            List<UnityRelation> newRelationships = new List<UnityRelation>(relationships);

            if (id < 0 || id >= relationships.Length)
            {
                entryID = newRelationships.Count;
                newRelationships.Add(new UnityRelation() { subject = subject });
            }
            else
            {
                newRelationships.Insert(id, new UnityRelation() { subject = subject });
                entryID = id;
            }

            relationships = newRelationships.ToArray();
            OnBeginRelationship?.Invoke(subject);

            return entryID;
        }

        public virtual int GetRelationshipID(UnityNarreme subject)
        {
            for(int i = relationships.Length-1; i >=0; i--)
            {
                if(relationships[i].subject == subject) { return i; }
            }
            return -1;
        }

        public virtual void EndRelationship<T>(T subject) where T:INarreme
        {
            if(subject is UnityNarreme narreme)
            {
                int id = GetRelationshipID(narreme);
                if(id >= 0) { EndRelationship(id); }
            }
        }

        public void EndRelationship(int toRemove)
        {
            UnityNarreme subject = relationships[toRemove].subject;
            List<UnityRelation> newRelationships = new List<UnityRelation>(relationships);
            newRelationships.RemoveAt(toRemove);
            relationships = newRelationships.ToArray();
            OnEndRelationship?.Invoke(subject);
        }

        #endregion

        #region Narreme Implementation

        public override ConditionStatus CheckNarrativeCondition<T>(int condition, T narremeToCheck = default)
        {
            if (narremeToCheck.Equals(default))
            {
                //Self Conditions
                switch (condition)
                {
                    case 0:
                        if (currentLocation != null) { return ConditionStatus.Met; }
                        return ConditionStatus.NotMet;
                    case 1:
                        if (relationships.Length > 0)
                        {
                            return ConditionStatus.Met;
                        }
                        return ConditionStatus.NotMet;
                    case 2:
                        if (goals.Length > 0)
                        {
                            return ConditionStatus.Met;
                        }
                        return ConditionStatus.NotMet;
                    case 3:
                        if (possessions.Length > 0)
                        {
                            return ConditionStatus.Met;
                        }
                        return ConditionStatus.NotMet;
                    case 4:
                        //Is Hidden. Hidden if character's current location is set,
                        // but it's not registered with that location.
                        if (currentLocation == null) { return ConditionStatus.NotMet; }
                        for (int i = currentLocation.presentCharacters.Length - 1; i >= 0; i--)
                        {
                            if (currentLocation.presentCharacters[i] == this) { return ConditionStatus.NotMet; }
                        }
                        return ConditionStatus.Met;
                }
            }
            else
            {
                if(condition == 0 && narremeToCheck is UnityNarreme nar)
                {
                    int relID = GetRelationshipID(nar);
                    if (relID >= 0) return ConditionStatus.Met;
                    return ConditionStatus.NotMet;
                }

                if(narremeToCheck is UnityCharacter character)
                {
                    switch (condition)
                    {
                        //Is
                        case 1:
                            if(character == this)return ConditionStatus.Met;
                            return ConditionStatus.NotMet;
                        //Is at same Location
                        case 2:
                            if (character.currentLocation == currentLocation) return ConditionStatus.Met;
                            return ConditionStatus.NotMet;
                    }
                }
                else if(narremeToCheck is UnityLocation location)
                {
                    switch (condition)
                    {
                        case 1:
                            if (location == currentLocation) return ConditionStatus.Met;
                            return ConditionStatus.NotMet;
                    }
                }
                else if(narremeToCheck is UnityProp prop)
                {
                    switch (condition)
                    {
                        case 1:
                            for (int i = possessions.Length - 1; i >= 0; i--)
                            {
                                if (prop == possessions[i]) { return ConditionStatus.Met; }

                            }
                            return ConditionStatus.NotMet;
                        case 2:
                            if (prop.currentLocation == currentLocation) { return ConditionStatus.Met; }
                            return ConditionStatus.NotMet;
                    }
                } 
                else if(narremeToCheck is UnityPlot plot)
                {
                    switch (condition)
                    {
                        case 1:
                            for (int i = goals.Length - 1; i >= 0; i--)
                            {
                                if (plot == goals[i]) { return ConditionStatus.Met; }

                            }
                            return ConditionStatus.NotMet;
                    }
                }
            }

            return ConditionStatus.NotMet;
        
    }

        public override T GetLiteralForm<T>()
        {
            if(narrative == null) { return default; }

            int characterID = narrative.GetCharacterOfRole(narrative.GetRoleID(this));
            if (characterID < 0) return default;
            if(narrative.characters[characterID] is T cha) { return cha; }
            return default;
        }

        public override T[] GetSymbolicForms<T>()
        {
            if (!typeof(UnityCharacter).IsAssignableFrom(typeof(T)))
            {
                return new T[0];
            }

            int[] symbolicIDs = narrative.GetRolesOfCharacter(narrative.GetCharacterID(this));
            UnityCharacter[] roles = new UnityCharacter[symbolicIDs.Length];
            for(int i = 0; i < roles.Length; i++)
            {
                roles[i] = narrative.roles[symbolicIDs[i]];
            }
            return roles as T[];
        }

        #endregion
    }

}
