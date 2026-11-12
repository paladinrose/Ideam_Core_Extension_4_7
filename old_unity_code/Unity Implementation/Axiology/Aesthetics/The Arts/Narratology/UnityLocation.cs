using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;
using Ideam.Metaphysics.Ontology.Existence;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    [CreateAssetMenu(fileName = "New Location", menuName = "Ideam/Narratology/Location")]
    public class UnityLocation : UnityNarreme, ILocation
    {
        public UnityCharacter[] presentCharacters;
        public UnityProp[] presentProps;

        public virtual event Action<UnityCharacter> OnCharacterEnter, OnCharacterExit;
        public virtual event Action<UnityProp> OnPropEnter, OnPropExit;

        public T[] GetThingsWithin<T>() where T : IThing
        {
            if (typeof(UnityCharacter).IsAssignableFrom(typeof(T)))
            {
                return presentCharacters as T[];
            }

            if (typeof(UnityProp).IsAssignableFrom(typeof(T)))
            {
                return presentProps as T[];
            }
            return new T[0];
        }

        #region Character Functions

        public virtual int EnterCharacter<T>(T character) where T:ICharacter
        {
            if (character is UnityCharacter chr)
            {
                return EnterCharacter(chr, -1);
            }
            return -1;
        }

        public virtual int EnterCharacter(UnityCharacter character, int id = -1)
        {
            int entryID = GetCharacterID(character);
            if (entryID >= 0) { return entryID; }


            List<UnityCharacter> newCharacters = new List<UnityCharacter>(presentCharacters);

            if (id < 0 || id >= presentCharacters.Length)
            {
                entryID = newCharacters.Count;
                newCharacters.Add(character);
            }
            else
            {
                newCharacters.Insert(id, character);
                entryID = id;
            }

            presentCharacters = newCharacters.ToArray();
            OnCharacterEnter?.Invoke(character);
            
            return entryID;
        }
        
        public int GetCharacterID(UnityCharacter character)
        {
            for(int i = presentCharacters.Length-1; i >=0; i--)
            {
                if(presentCharacters[i] == character) { return i; }
            }
            return -1;
        }

        public virtual void ExitCharacter<T>(T character) where T:ICharacter
        {
            if (character is UnityCharacter chr)
            {
                int id = GetCharacterID(chr);
                if (id >= 0) { ExitCharacter(id); }
            }
        }

        public virtual void ExitCharacter(int characterID)
        {
            UnityCharacter ch = presentCharacters[characterID];
            List<UnityCharacter> newCharacters = new List<UnityCharacter>(presentCharacters);

            newCharacters.RemoveAt(characterID);

            presentCharacters = newCharacters.ToArray();
            OnCharacterExit?.Invoke(ch);
            
        }

        #endregion

        #region Prop Functions

        public virtual int GetProp<T>(T prop) where T : IProp
        {
            if (prop is UnityProp newProp)
            {
                return GetProp(newProp,  -1);
            }
            return -1;
        }

        public virtual int GetProp(UnityProp prop, int id = -1)
        {
            int entryID = GetPropID(prop);
            if (entryID >= 0) { return entryID; }

            List<UnityProp> newProps = new List<UnityProp>(presentProps);

            if (id < 0 || id >= presentProps.Length)
            {
                entryID = newProps.Count;
                newProps.Add(prop);
            }
            else
            {
                newProps.Insert(id, prop);
                entryID = id;
            }

            presentProps = newProps.ToArray();
            if(prop.currentLocation != null)
            {
                prop.currentLocation.LoseProp(prop);
            }

            prop.currentLocation = this;

            OnPropEnter?.Invoke(prop);
            
            return entryID;
        }
        
        public virtual int GetPropID(UnityProp prop)
        {
            for(int i = presentProps.Length-1; i >=0; i--)
            {
                if(presentProps[i] == prop) { return i; }
            }
            return -1;
        }
        public virtual void LoseProp<T>(T obj) where T:IProp
        {
            if(obj is UnityProp lost)
            {
                int id = GetPropID(lost);
                if(id >= 0) { LoseProp(id); }
            }
        }
        public virtual void LoseProp(int id)
        {
            UnityProp losing = presentProps[id];
            List<UnityProp> newProps = new List<UnityProp>(presentProps);
            newProps.RemoveAt(id);
            presentProps = newProps.ToArray();
            OnPropExit?.Invoke(losing);
            
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
                        if (presentCharacters.Length == 0 && presentProps.Length == 0) { return ConditionStatus.Met; }
                        return ConditionStatus.NotMet;
                    case 1:
                        if (presentCharacters.Length > 0)
                        {
                            return ConditionStatus.Met;
                        }
                        return ConditionStatus.NotMet;
                    case 2:
                        if (presentProps.Length > 0)
                        {
                            return ConditionStatus.Met;
                        }
                        return ConditionStatus.NotMet;
                }
            }
            else
            {
                if (narremeToCheck is UnityCharacter character)
                {
                    switch (condition)
                    {
                        //Present
                        case 0:
                            if (GetCharacterID(character) >=0) return ConditionStatus.Met;
                            return ConditionStatus.NotMet;
                       
                    }
                }
                else if (narremeToCheck is UnityLocation location)
                {
                    switch (condition)
                    {
                        case 0:
                            if (location == this) return ConditionStatus.Met;
                            return ConditionStatus.NotMet;
                    }
                }
                else if (narremeToCheck is UnityProp obj)
                {
                    switch (condition)
                    {
                        //Present
                        case 0:
                            if (GetPropID(obj) >= 0) return ConditionStatus.Met;
                            return ConditionStatus.NotMet;

                    }
                }
            }

            return ConditionStatus.NotMet;

        }

        public override T GetLiteralForm<T>()
        {
            int locationID = narrative.GetLocationInPlace(narrative.GetPlaceID(this));
            if (locationID < 0) return default;
            if (narrative.locations[locationID] is T cha) { return cha; }
            return default;
        }

        public override T[] GetSymbolicForms<T>()
        {
            if (!typeof(UnityLocation).IsAssignableFrom(typeof(T)))
            {
                return new T[0];
            }

            int[] symbolicIDs = narrative.GetPlacesForLocation(narrative.GetLocationID(this));
            UnityLocation[] places = new UnityLocation[symbolicIDs.Length];
            for (int i = 0; i < places.Length; i++)
            {
                places[i] = narrative.places[symbolicIDs[i]];
            }
            return places as T[];
        }

        #endregion

    }

}
