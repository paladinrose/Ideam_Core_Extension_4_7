using System;

using UnityEngine;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    [CreateAssetMenu(fileName = "New Prop", menuName = "Ideam/Narratology/Prop")]
    public class UnityProp : UnityNarreme, IProp
    {
        public UnityLocation currentLocation;
        public UnityCharacter currentPossessor;

        public virtual event Action<UnityLocation> OnMoveToLocation, OnLeaveLocation;
        public virtual event Action<UnityCharacter> OnPassedTo, OnLost;
        public virtual event Action<UnityIncident> OnCause;

        public virtual event Action OnBecomeLost;

        public virtual bool MoveToLocation<T>(T newLocation) where T : ILocation
        {
            if(newLocation is UnityLocation loc)
            {
                if(loc == currentLocation) { return false; }

                if(currentLocation != null)
                {
                    OnLeaveLocation?.Invoke(currentLocation);
                }

                currentLocation = loc;
                OnMoveToLocation?.Invoke(currentLocation);
                return true;
            }
            return false;
        }

        public virtual bool PassTo<T>(T character) where T:ICharacter
        {
            if (character is UnityCharacter newPossessor)
            {
                if (currentPossessor != null)
                {
                    currentPossessor.LosePossession(this);
                    OnLost?.Invoke(currentPossessor);
                }

                newPossessor.TakePossession(this);
                OnPassedTo?.Invoke(newPossessor);

                return true;
            }

            return false;
        }

        public virtual bool CauseEvent<T>(T evnt) where T : IIncident
        {
            if(evnt is UnityIncident eventual)
            {
                if (eventual.Status == IncidentStatus.HasHappened)
                {
                    OnCause?.Invoke(eventual);
                    return true;
                }
            }
            return false;
        }

        public virtual void BecomeLost()
        {
            if (currentPossessor != null)
            {
                currentPossessor.LosePossession(this);
                currentPossessor = null;
            }
            if (currentLocation != null)
            {
                currentLocation.LoseProp(this);
                currentLocation = null;
            }

            OnBecomeLost?.Invoke();
            
        }


        #region Narreme Implementation

        public override ConditionStatus CheckNarrativeCondition<T>(int condition, T narremeToCheck = default)
        {
            if (narremeToCheck.Equals(default))
            {
                //Self Conditions
                switch (condition)
                {
                    case 0:
                        //Is Lost
                        if (currentLocation == null && currentPossessor == null)
                        {
                            return ConditionStatus.Met;
                        }
                        return ConditionStatus.NotMet;
                    case 1:
                        //Has Possessor
                        if (currentPossessor != null)
                        {
                            return ConditionStatus.Met;
                        }
                        return ConditionStatus.NotMet;
                    case 2:
                        //Is Somewhere
                        if (currentLocation != null)
                        {
                            return ConditionStatus.Met;
                        }
                        return ConditionStatus.NotMet;
                    case 3:
                        //Is Hidden.
                        if (currentLocation == null) { return ConditionStatus.NotMet; }
                        for (int i = currentLocation.presentProps.Length - 1; i >= 0; i--)
                        {
                            if (currentLocation.presentProps[i] == this) { return ConditionStatus.NotMet; }
                        }
                        return ConditionStatus.Met;
                }
            }
            else
            {
                if (narremeToCheck is UnityCharacter character)
                {
                    switch (condition)
                    {
                        case 0:
                            //Is Possessed By
                            if (character == currentPossessor)
                            {
                                return ConditionStatus.Met;
                            }
                            return ConditionStatus.NotMet;
                        case 1:
                            if (currentLocation == null) { return ConditionStatus.CannotMeet; }

                            if (character.currentLocation == currentLocation) { return ConditionStatus.Met; }
                            return ConditionStatus.NotMet;
                    }
                }
                else if (narremeToCheck is UnityLocation location)
                {
                    switch (condition)
                    {
                        case 1:
                            if (location == currentLocation) return ConditionStatus.Met;
                            return ConditionStatus.NotMet;
                    }
                }
                else if (narremeToCheck is UnityProp obj)
                {
                    switch (condition)
                    {
                        case 0:
                            if (obj == this)
                            {
                                return ConditionStatus.Met;
                            }
                            return ConditionStatus.NotMet;
                        case 1:
                            if (obj.currentLocation == currentLocation) { return ConditionStatus.Met; }
                            return ConditionStatus.NotMet;
                        case 2:
                            if (obj.currentPossessor == currentPossessor) { return ConditionStatus.Met; }
                            return ConditionStatus.NotMet;
                    }
                }
                
            }

            return ConditionStatus.NotMet;

        }

        public override T GetLiteralForm<T>()
        {
            int objectID = narrative.GetSymbolizedObject(narrative.GetSymbolID(this));
            if (objectID < 0) return default;
            if (narrative.objects[objectID] is T cha) { return cha; }
            return default;
        }

        public override T[] GetSymbolicForms<T>()
        {
            if (!typeof(UnityProp).IsAssignableFrom(typeof(T)))
            {
                return new T[0];
            }

            int[] symbolicIDs = narrative.GetObjectifiedSymbols(narrative.GetObjectID(this));
            UnityProp[] symbols = new UnityProp[symbolicIDs.Length];
            for (int i = 0; i < symbols.Length; i++)
            {
                symbols[i] = narrative.symbols[symbolicIDs[i]];
            }
            return symbols as T[];
        }

        #endregion
    }

}
