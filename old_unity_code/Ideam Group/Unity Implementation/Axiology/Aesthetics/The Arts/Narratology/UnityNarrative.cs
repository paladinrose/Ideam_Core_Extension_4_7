using System;
using System.Collections;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Linq;

using UnityEngine;

using Ideam.Axiology.Aesthetics.The_Arts.Narratology;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Science.Formal.Mathematics;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology
{
    [CreateAssetMenu(fileName = "New Narrative", menuName = "Ideam/Narratology/Narrative")]
    public class UnityNarrative : UnityNarreme, INarrative
    {
        protected bool enabled = false;

        public List<NarrativeText> narrativeTexts = new List<NarrativeText>();

        public UnityNarreme GetNarremeFromSymbolName(string narremeName)
        {
            
            for (int i = roles.Length - 1; i >= 0; i--)
            {
                if (roles[i].name == narremeName)
                {
                    if (characterToRoleAssignments[i] >= 0) 
                    { return characters[characterToRoleAssignments[i]]; }
                    return default;
                }
            }
            
            for (int i = places.Length - 1; i >= 0; i--)
            {
                if (places[i].name == narremeName)
                {
                    if (locationToPlaceAssignments[i] >= 0) 
                    { return locations[locationToPlaceAssignments[i]]; }
                    return null;
                }
            }

            for (int i = symbols.Length - 1; i >= 0; i--)
            {
                if (symbols[i].name == narremeName)
                {
                    if (objectToSymbolAssignments[i] >= 0) 
                    { return objects[objectToSymbolAssignments[i]]; }
                    return null;
                }
            }


            for (int i = beats.Length - 1; i >= 0; i--)
            {
                if (beats[i].name == narremeName)
                {
                    if (eventToBeatAssignments[i] >= 0) 
                    { return events[eventToBeatAssignments[i]]; }
                    return null;
                }
            }

            for (int i = stories.Length - 1; i >= 0; i--)
            {
                if (stories[i].name == narremeName)
                {
                    if (plotToStoryAssignments[i] >= 0) 
                    { return plots[plotToStoryAssignments[i]]; }
                    return null;
                }
            }

            return null;
        }
        
        #region INarrative Implementation

        #region Characters and Roles

        public UnityCharacter[] characters = new UnityCharacter[0],
                           roles = new UnityCharacter[0];

        public int[] characterToRoleAssignments = new int[0];


        public virtual event Action<ICharacter> OnAddCharacter, OnRemoveCharacter,
                                                OnAddRole, OnRemoveRole;

        public virtual event Action<int, int> OnSetCharacterToRole,
                                              OnRemoveCharacterFromRole;


        public virtual int AddCharacter<T>(T newCharacter) where T : ICharacter
        {
            if (newCharacter is UnityCharacter character)
            {
                return AddCharacter(character, -1);
            }
            return -1;
        }

        public virtual int AddCharacter(UnityCharacter newCharacter, int id = -1)
        {
            int entryID = GetCharacterID(newCharacter);
            if (entryID < 0)
            {
                

                List<UnityCharacter> newCharacters = new List<UnityCharacter>(characters);

                if (id < 0 || id >= characters.Length)
                {
                    entryID = newCharacters.Count;
                    newCharacters.Add(newCharacter);
                }
                else
                {
                    newCharacters.Insert(id, newCharacter);
                    entryID = id;

                    if(id < characters.Length - 1)
                    {
                        for (int i = characterToRoleAssignments.Length - 1; i >= 0; i--)
                        {
                            if (characterToRoleAssignments[i] >=id)
                            {
                                characterToRoleAssignments[i]++;
                            }
                        }
                    }
                }

                characters = newCharacters.ToArray();
                OnAddCharacter?.Invoke(newCharacter);
            }
            return entryID;
        }

        public virtual int GetCharacterID(UnityCharacter character)
        {
            for (int i = 0; i < characters.Length; i++)
            {
                if (characters[i] == character) { return i; }
            }

            return -1;
        }
        
        public virtual void RemoveCharacter<T>(T toRemove) where T : ICharacter
        {
            if(toRemove is UnityCharacter character)
            {
                int id = GetCharacterID(character);
                if(id >= 0) { RemoveCharacter(id); }
            }
        }
        
        public virtual void RemoveCharacter(int toRemove)
        {
            if (toRemove >= 0 && toRemove < characters.Length)
            {
                for(int i = characterToRoleAssignments.Length-1; i >=0; i--)
                {
                    if(characterToRoleAssignments[i] > toRemove) 
                    { characterToRoleAssignments[i]--; }
                    else if(characterToRoleAssignments[i] == toRemove) { RemoveCharacterFromRole(i); }
                }

                UnityCharacter character = characters[toRemove];
                List<UnityCharacter> newCharacters = new List<UnityCharacter>(characters);
                newCharacters.RemoveAt(toRemove);
                characters = newCharacters.ToArray();
                OnRemoveCharacter?.Invoke(character);
            }
        }


        public virtual int AddRole<T>(T newRole) where T : ICharacter
        {
            if (newRole is UnityCharacter role)
            {
                return AddRole(role, -1);
            }
            return -1;
        }

        public virtual int AddRole(UnityCharacter newRole, int id = -1)
        {
            int entryID = GetRoleID(newRole);
            if (entryID < 0)
            {
                List<UnityCharacter> newRoles = new List<UnityCharacter>(roles);
                List<int> newCharacterToRoleAssignments = new List<int>();

                if (id < 0 || id >= roles.Length)
                {
                    entryID = newRoles.Count;

                    newRoles.Add(newRole);
                    newCharacterToRoleAssignments.Add(-1);
                }
                else
                {
                    newRoles.Insert(id, newRole);
                    newCharacterToRoleAssignments.Insert(id, -1);

                    entryID = id;
                }

                roles = newRoles.ToArray();
                characterToRoleAssignments = newCharacterToRoleAssignments.ToArray();

                OnAddRole?.Invoke(newRole);

                return roles.Length - 1;
            }
            return entryID;
        }

        public virtual int GetRoleID(UnityCharacter role)
        {
            for (int i = 0; i < roles.Length; i++)
            {
                if (roles[i] == role ) { return i; }
            }
            return -1;
        }

        public virtual void RemoveRole<T>(T toRemove) where T : ICharacter
        {
            if (toRemove is UnityCharacter role)
            {
                int id = GetRoleID(role);
                if (id >= 0) { RemoveRole(id); }
            }
        }

        public virtual void RemoveRole(int toRemove) 
        {
            if (toRemove >= 0 && toRemove < roles.Length)
            {
                UnityCharacter role = roles[toRemove];
                RemoveCharacterFromRole(toRemove);

                List<UnityCharacter> newRoles = new List<UnityCharacter>(roles);
                newRoles.RemoveAt(toRemove);
                roles = newRoles.ToArray();

                List<int> newCharacterToRoleAssignments = new List<int>(characterToRoleAssignments);
                newCharacterToRoleAssignments.RemoveAt(toRemove);
                characterToRoleAssignments = newCharacterToRoleAssignments.ToArray();

                OnRemoveRole?.Invoke(role);
            }
        }


        public virtual int GetBestCharacterFitForRole(int roleID, int[] masked, BestFit bestFit = BestFit.Basic) 
        {
            if(roleID <0 || roleID >= roles.Length) { return -1; }

            int highScore = int.MaxValue, highID = -1;
            Int score = new Int(0);

            UnityCharacter roleToFill  = roles[roleID];
            UnityCharacter currentCharacterInRole;
            int currentCharacterID = characterToRoleAssignments[roleID];
        
            if(currentCharacterID >= 0) { currentCharacterInRole = characters[currentCharacterID]; }

            switch (bestFit)
            {
                case BestFit.Basic:
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        //Removing this check. If you don't want to
                        //include the current character, you can get the ID of the
                        //current character in the role and put it in the mask list.
                        //if (i == currentCharacterID) { continue; }

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        if (roleToFill.semantics.CompareSemanticAffinity<Int>(characters[i].semantics, out score))
                        {
                            if (score.x < highScore)
                            {
                                highScore = score.x;
                                highID = i;
                            }
                        }
                    }
                    break;

                case BestFit.Symbolic:
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }

                        roleToFill.semantics.CompareSemanticAffinity<Int>(characters[i].semantics, out score);

                        if (roleToFill.currentLocation != null)
                        {
                            UnityLocation originalPlace = roleToFill.currentLocation;
                            int placeID = GetPlaceID(originalPlace);
                            
                            if (locationToPlaceAssignments[placeID] >=0 &&
                                locations[locationToPlaceAssignments[placeID]] != characters[i].currentLocation)
                            {
                                score++;
                            }
                        }
                        else if (characters[i].currentLocation != null)
                        {
                            score++;
                        }

                        for (int j = roleToFill.relationships.Length - 1; j >= 0; j--)
                        {
                            bool hasTitle = false, hasNarreme = false;
                            for (int k = characters[i].relationships.Length - 1; k >= 0; k--)
                            {
                                if (characters[i].relationships[k].title == roleToFill.relationships[j].title)
                                {
                                    hasTitle = true;
                                    if (roleToFill.relationships[j].subject != null)
                                    {
                                        UnityNarreme roleRelation = GetNarremeFromSymbolName(roleToFill.relationships[j].title);
                                        if (roleRelation != null && roleRelation == characters[i].relationships[j].subject)
                                        {
                                            hasNarreme = true;
                                        }
                                    }
                                    else if (characters[i].relationships[k].subject == null)
                                    {
                                        hasNarreme = true;
                                    }
                                    break;
                                }
                            }
                            if (!hasTitle) { score += 2; }
                            else if (!hasNarreme) { score++; }
                        }

                        for (int j = roleToFill.possessions.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;

                            UnityProp roleSymbol = roleToFill.possessions[j];
                            int posID = GetSymbolID(roleSymbol);

                            if (objectToSymbolAssignments[posID] >=0)
                            {
                                UnityProp currentPossession = objects[objectToSymbolAssignments[posID]];

                                for (int k = characters[i].possessions.Length - 1; k >= 0; k--)
                                {
                                    if (characters[i].possessions[k] == currentPossession)
                                    {
                                        hasNarreme = true;
                                        break;
                                    }
                                }
                                if (!hasNarreme) { score++; }
                            }
                        }

                        for (int j = roleToFill.goals.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;

                            UnityPlot roleStory = roleToFill.goals[j];
                            int stID = GetStoryID(roleStory);
                            if (plotToStoryAssignments[stID] >= 0)
                            {
                                UnityPlot currentGoal = plots[plotToStoryAssignments[stID]];
                                
                                for (int k = characters[i].goals.Length - 1; k >= 0; k--)
                                {
                                    if (characters[i].goals[k] == currentGoal)
                                    {
                                        hasNarreme = true;
                                        break;
                                    }
                                }

                                
                                if (!hasNarreme) { score++; }
                            }
                        }

                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Literal:
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        roleToFill.semantics.CompareSemanticAffinity<Int>(characters[i].semantics, out score);

                        if (roleToFill.currentLocation != characters[i].currentLocation)
                        { score++; }

                        for (int j = roleToFill.relationships.Length - 1; j >= 0; j--)
                        {
                            bool hasTitle = false, hasNarreme = false;
                            for (int k = characters[i].relationships.Length - 1; k >= 0; k--)
                            {
                                if (characters[i].relationships[k].title == roleToFill.relationships[j].title)
                                {
                                    hasTitle = true;
                                    if (roleToFill.relationships[j].subject != null &&
                                        characters[i].relationships[k].subject == roleToFill.relationships[j].subject)
                                    {
                                        hasNarreme = true;
                                    }
                                    else if (roleToFill.relationships[j].subject == null &&
                                        characters[i].relationships[k].subject == null)
                                    {
                                        hasNarreme = true;
                                    }
                                    break;
                                }
                            }
                            if (!hasTitle) { score += 2; }
                            else if (!hasNarreme) { score++; }
                        }

                        for (int j = roleToFill.possessions.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;
                            for (int k = characters[i].possessions.Length - 1; k >= 0; k--)
                            {
                                if (characters[i].possessions[k] == roleToFill.possessions[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }
                            }
                            if (!hasNarreme) { score++; }
                        }

                        for (int j = roleToFill.goals.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;
                            for (int k = characters[i].goals.Length - 1; k >= 0; k--)
                            {
                                if (characters[i].goals[k] == roleToFill.goals[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }

                            }
                            if (!hasNarreme) { score++; }
                        }

                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Thorough:
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        roleToFill.semantics.CompareSemanticAffinity<Int>(characters[i].semantics, out score);

                        if (roleToFill.currentLocation != null)
                        {
                            if (roleToFill.currentLocation != characters[i].currentLocation)
                            {
                                int locID = GetPlaceID(roleToFill.currentLocation);

                                if (locID <0 || locations[locationToPlaceAssignments[locID]] != characters[i].currentLocation)
                                {
                                    score++;
                                }
                            }
                        }
                        else if (characters[i].currentLocation != null)
                        {
                            score++;
                        }

                        for (int j = roleToFill.relationships.Length - 1; j >= 0; j--)
                        {
                            bool hasTitle = false, hasNarreme = false;
                            for (int k = characters[i].relationships.Length - 1; k >= 0; k--)
                            {
                                if (characters[i].relationships[k].title == roleToFill.relationships[j].title)
                                {
                                    hasTitle = true;
                                    if (roleToFill.relationships[j].subject != null)
                                    {
                                        if (roleToFill.relationships[j].subject == characters[i].relationships[k].subject)
                                        {
                                            hasNarreme = true;
                                        }
                                        else
                                        {
                                            UnityNarreme roleRelation = GetNarremeFromSymbolName(roleToFill.relationships[j].title);
                                            if (roleRelation != null && roleRelation == characters[i].relationships[j].subject)
                                            {
                                                hasNarreme = true;
                                            }
                                        }
                                    }
                                    else if (characters[i].relationships[k].subject == null)
                                    {
                                        hasNarreme = true;
                                    }
                                    break;
                                }
                            }
                            if (!hasTitle) { score += 2; }
                            else if (!hasNarreme) { score++; }
                        }

                        for (int j = roleToFill.possessions.Length - 1; j >= 0; j--)
                        {

                                bool hasNarreme = false;
                                for (int k = characters[i].possessions.Length - 1; k >= 0; k--)
                                {
                                    if (characters[i].possessions[k] == roleToFill.possessions[j])
                                    {
                                        hasNarreme = true;
                                        break;
                                    }

                                }
                                if (!hasNarreme)
                                {
                                    int posID = GetSymbolID(roleToFill.possessions[j]);
                                    if (posID >=0 && objectToSymbolAssignments[posID] >=0)
                                    {
                                        for (int k = characters[i].possessions.Length - 1; k >= 0; k--)
                                        {
                                            if (characters[i].possessions[k] == objects[objectToSymbolAssignments[posID]])
                                            {
                                                hasNarreme = true;
                                                break;
                                            }
                                        }
                                    }
                                }

                                if (!hasNarreme) { score++; }
                            
                        }

                        for (int j = roleToFill.goals.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;
                            for (int k = characters[i].goals.Length - 1; k >= 0; k--)
                            {
                                if (characters[i].goals[k] == roleToFill.goals[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }

                            }
                            if (!hasNarreme)
                            {
                                int plotID = GetStoryID(roleToFill.goals[j]);
                                if (plotID >=0 && plotToStoryAssignments[plotID] >=0)
                                {
                                    for (int k = characters[i].goals.Length - 1; k >= 0; k--)
                                    {
                                        if (characters[i].goals[k] == plots[plotToStoryAssignments[plotID]])
                                        {
                                            hasNarreme = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!hasNarreme) { score++; }
                            
                        }
                        if (score.x < highScore) { highScore = score.x; highID = i; }
                    }
                    break;
            }

            return highID;
        }
        
        public virtual bool SetCharacterToRole(int characterID, int roleID, ReplaceType replacement = ReplaceType.None) 
        {
            UnityCharacter currentCharacterInRole = null;
            if (characterToRoleAssignments[roleID] >= 0)
            {
                currentCharacterInRole = characters[characterToRoleAssignments[roleID]];
            }

            switch (replacement)
            {
                case ReplaceType.Symbolic:

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].title == roles[roleID].name ||
                                characters[i].relationships[j].subject == roles[roleID])
                            {
                                characters[i].relationships[j].subject= characters[characterID];
                            }
                        }
                    }

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalTitles[j] == roles[roleID].name ||
                                events[i].conditionalNarremes[j] == roles[roleID])
                            {
                                events[i].conditionalNarremes[j] = characters[characterID];
                            }
                        }
                    }
                    break;

                case ReplaceType.Literal:
                    if (currentCharacterInRole == null) { break; }

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].subject == currentCharacterInRole)
                            {
                                characters[i].relationships[j].subject = characters[characterID];
                            }
                        }
                    }

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalNarremes[j] == currentCharacterInRole)
                            {
                                events[i].conditionalNarremes[j] = characters[characterID];
                            }
                        }
                    }
                    break;
                case ReplaceType.Thorough:
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if ((currentCharacterInRole != null && characters[i].relationships[j].subject == currentCharacterInRole) ||
                                characters[i].relationships[j].subject == roles[roleID] ||
                                characters[i].relationships[j].title == roles[roleID].name)
                            {
                                characters[i].relationships[j].subject = characters[characterID];
                            }
                        }
                    }


                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if ((currentCharacterInRole != null && events[i].conditionalNarremes[j] == currentCharacterInRole) ||
                                events[i].conditionalNarremes[j] == roles[roleID] ||
                                events[i].conditionalTitles[j] == roles[roleID].name)
                            {
                                events[i].conditionalNarremes[j] = characters[characterID];
                            }
                        }
                    }
                    break;
            }

            characterToRoleAssignments[roleID] = characterID;
            
            OnSetCharacterToRole?.Invoke(characterID, roleID);

            return true;
        }
        
        public virtual void RemoveCharacterFromRole(int roleID) 
        {
            if (characterToRoleAssignments[roleID] < 0) { return; }
            
            int characterID = characterToRoleAssignments[roleID];

            UnityCharacter currentCharacterInRole = characters[characterToRoleAssignments[roleID]];
            
            for (int i = characters.Length - 1; i >= 0; i--)
            {
                for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                {
                    if ((characters[i].relationships[j].subject == currentCharacterInRole) &&
                        characters[i].relationships[j].title == roles[roleID].name)
                    {
                        characters[i].relationships[j].subject = null;
                    }
                }
            }

            for (int i = events.Length - 1; i >= 0; i--)
            {
                for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                {
                    if ((events[i].conditionalNarremes[j] == currentCharacterInRole) &&
                        events[i].conditionalTitles[j] == roles[roleID].name)
                    {
                        events[i].conditionalNarremes[j] = null;
                    }
                }
            }
            characterToRoleAssignments[roleID] = -1;
            OnRemoveCharacterFromRole?.Invoke(characterID, roleID);
        }

        public virtual int GetCharacterOfRole(int roleID){ return characterToRoleAssignments[roleID]; }
        
        public virtual int[] GetRolesOfCharacter(int characterID)
        {
            List<int> roleIDs = new List<int>();
            for(int i = 0; i < roles.Length; i++)
            {
                if(characterToRoleAssignments[i] == characterID)
                {
                    roleIDs.Add(i);
                }
            }
            return roleIDs.ToArray();
        }

        #endregion

        #region Locations and Places

        public UnityLocation[] locations = new UnityLocation[0],
                           places = new UnityLocation[0];

        public int[] locationToPlaceAssignments = new int[0];


        public virtual event Action<ILocation> OnAddLocation, OnRemoveLocation,
                                                OnAddPlace, OnRemovePlace;

        public virtual event Action<int, int> OnSetLocationToPlace,
                                              OnRemoveLocationFromPlace;


        public virtual int AddLocation<T>(T newLocation) where T : ILocation
        {
            if (newLocation is UnityLocation location)
            {
                return AddLocation(location, -1);
            }
            return -1;
        }
        public virtual int AddLocation(UnityLocation newLocation, int id = -1)
        {
            int entryID = GetLocationID(newLocation);
            if (entryID < 0)
            {
                List<UnityLocation> newLocations = new List<UnityLocation>(locations);

                if (id < 0 || id >= locations.Length)
                {
                    entryID = newLocations.Count;
                    newLocations.Add(newLocation);
                }
                else
                {
                    newLocations.Insert(id, newLocation);
                    entryID = id;

                    if (id < locations.Length - 1)
                    {
                        for (int i = locationToPlaceAssignments.Length - 1; i >= 0; i--)
                        {
                            if (locationToPlaceAssignments[i] >= id)
                            {
                                locationToPlaceAssignments[i]++;
                            }
                        }
                    }
                }

                locations = newLocations.ToArray();
                OnAddLocation?.Invoke(newLocation);

            }
            return entryID;
        }

        public virtual int GetLocationID(UnityLocation location)
        {
            for (int i = 0; i < locations.Length; i++)
            {
                if (locations[i] == location) { return i; }
            }

            return -1;
        }

        public virtual void RemoveLocation<T>(T toRemove) where T : ILocation
        {
            if (toRemove is UnityLocation location)
            {
                int id = GetLocationID(location);
                if (id >= 0) { RemoveLocation(id); }
            }
        }
        public virtual void RemoveLocation(int toRemove)
        {
            if (toRemove >= 0 && toRemove < locations.Length)
            {
                for (int i = locationToPlaceAssignments.Length - 1; i >= 0; i--)
                {
                    if (locationToPlaceAssignments[i] > toRemove)
                    { locationToPlaceAssignments[i]--; }
                    else if (locationToPlaceAssignments[i] == toRemove) { RemoveLocationFromPlace(i); }
                }
                UnityLocation location = locations[toRemove];
                List<UnityLocation> newLocations = new List<UnityLocation>(locations);
                newLocations.RemoveAt(toRemove);
                locations = newLocations.ToArray();
                OnRemoveLocation?.Invoke(location);
            }
        }


        public virtual int AddPlace<T>(T newPlace) where T : ILocation
        {
            if (newPlace is UnityLocation place)
            {
                return AddPlace(place, -1);
            }
            return -1;
        }
        public virtual int AddPlace(UnityLocation newPlace, int id = -1)
        {
            int entryID = GetPlaceID(newPlace);
            if (entryID < 0)
            {
                List<UnityLocation> newPlaces = new List<UnityLocation>(places);
                List<int> newLocationToPlaceAssignments = new List<int>(locationToPlaceAssignments);

                if (id < 0 || id >= places.Length)
                {
                    entryID = newPlaces.Count;

                    newPlaces.Add(newPlace);
                    newLocationToPlaceAssignments.Add(-1);
                }
                else
                {
                    newPlaces.Insert(id, newPlace);
                    newLocationToPlaceAssignments.Insert(id, -1);

                    entryID = id;
                }

                places = newPlaces.ToArray();
                locationToPlaceAssignments = newLocationToPlaceAssignments.ToArray();

                OnAddPlace?.Invoke(newPlace);

                return places.Length - 1;
            }
            return entryID;
        }

        public virtual int GetPlaceID(UnityLocation place)
        {
            for (int i = 0; i < places.Length; i++)
            {
                if (places[i] == place) { return i; }
            }
            return -1;
        }

        public virtual void RemovePlace<T>(T toRemove) where T : ILocation
        {
            if (toRemove is UnityLocation place)
            {
                int id = GetPlaceID(place);
                if (id >= 0) { RemovePlace(id); }
            }
        }
        public virtual void RemovePlace(int toRemove)
        {
            if (toRemove >= 0 && toRemove < places.Length)
            {
                UnityLocation place = places[toRemove];
                RemoveLocationFromPlace(toRemove);

                List<UnityLocation> newPlaces = new List<UnityLocation>(places);
                newPlaces.RemoveAt(toRemove);
                places = newPlaces.ToArray();


                List<int> newLocationToPlaceAssignments = new List<int>(locationToPlaceAssignments);
                newLocationToPlaceAssignments.RemoveAt(toRemove);
                locationToPlaceAssignments = newLocationToPlaceAssignments.ToArray();

                OnRemovePlace?.Invoke(place);
            }
        }

        public virtual int GetBestLocationFitForPlace(int placeID, int[] masked, BestFit bestFit = BestFit.Basic)
        {
            if (placeID < 0 || placeID >= places.Length) { return -1; }

            int highScore = int.MaxValue, highID = -1;
            Int score = new Int(0);

            UnityLocation placeToFill = places[placeID];
            UnityLocation currentLocationInPlace;
            int currentLocationID = locationToPlaceAssignments[placeID];

            if (currentLocationID >= 0) { currentLocationInPlace = locations[currentLocationID]; }

            switch (bestFit)
            {
                case BestFit.Basic:
                    for (int i = locations.Length - 1; i >= 0; i--)
                    {
                        //Removing this check. If you don't want to
                        //include the current location, you can get the ID of the
                        //current location in the place and put it in the mask list.
                        //if (i == currentLocationID) { continue; }

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        if (placeToFill.semantics.CompareSemanticAffinity<Int>(locations[i].semantics, out score))
                        {
                            if (score.x < highScore)
                            {
                                highScore = score.x;
                                highID = i;
                            }
                        }
                    }
                    break;

                case BestFit.Symbolic:
                    for (int i = locations.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }

                        placeToFill.semantics.CompareSemanticAffinity<Int>(locations[i].semantics, out score);


                        for (int j = placeToFill.presentProps.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;

                            UnityProp placeSymbol = placeToFill.presentProps[j];
                            int posID = GetSymbolID(placeSymbol);

                            if (objectToSymbolAssignments[posID] >= 0)
                            {
                                UnityProp currentPresentObject = objects[objectToSymbolAssignments[posID]];

                                for (int k = locations[i].presentProps.Length - 1; k >= 0; k--)
                                {
                                    if (locations[i].presentProps[k] == currentPresentObject)
                                    {
                                        hasNarreme = true;
                                        break;
                                    }
                                }
                                if (!hasNarreme) { score++; }
                            }
                        }

                        for (int j = placeToFill.presentCharacters.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;

                            UnityCharacter placeCharacter = placeToFill.presentCharacters[j];
                            int stID = GetCharacterID(placeCharacter);
                            if (characterToRoleAssignments[stID] >= 0)
                            {
                                UnityCharacter currentPresentCharacter = characters[characterToRoleAssignments[stID]];

                                for (int k = locations[i].presentCharacters.Length - 1; k >= 0; k--)
                                {
                                    if (locations[i].presentCharacters[k] == currentPresentCharacter)
                                    {
                                        hasNarreme = true;
                                        break;
                                    }
                                }


                                if (!hasNarreme) { score++; }
                            }
                        }

                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Literal:
                    for (int i = locations.Length - 1; i >= 0; i--)
                    {
                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        placeToFill.semantics.CompareSemanticAffinity<Int>(locations[i].semantics, out score);

                        for (int j = placeToFill.presentProps.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;
                            for (int k = locations[i].presentProps.Length - 1; k >= 0; k--)
                            {
                                if (locations[i].presentProps[k] == placeToFill.presentProps[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }
                            }
                            if (!hasNarreme) { score++; }
                        }

                        for (int j = placeToFill.presentCharacters.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;
                            for (int k = locations[i].presentCharacters.Length - 1; k >= 0; k--)
                            {
                                if (locations[i].presentCharacters[k] == placeToFill.presentCharacters[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }

                            }
                            if (!hasNarreme) { score++; }
                        }

                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Thorough:
                    for (int i = locations.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }

                        placeToFill.semantics.CompareSemanticAffinity<Int>(locations[i].semantics, out score);

                        for (int j = placeToFill.presentProps.Length - 1; j >= 0; j--)
                        {

                            bool hasNarreme = false;
                            for (int k = locations[i].presentProps.Length - 1; k >= 0; k--)
                            {
                                if (locations[i].presentProps[k] == placeToFill.presentProps[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }

                            }
                            if (!hasNarreme)
                            {
                                int posID = GetSymbolID(placeToFill.presentProps[j]);
                                if (posID >= 0 && objectToSymbolAssignments[posID] >= 0)
                                {
                                    for (int k = locations[i].presentProps.Length - 1; k >= 0; k--)
                                    {
                                        if (locations[i].presentProps[k] == objects[objectToSymbolAssignments[posID]])
                                        {
                                            hasNarreme = true;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (!hasNarreme) { score++; }

                        }

                        for (int j = placeToFill.presentCharacters.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;
                            for (int k = locations[i].presentCharacters.Length - 1; k >= 0; k--)
                            {
                                if (locations[i].presentCharacters[k] == placeToFill.presentCharacters[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }

                            }
                            if (!hasNarreme)
                            {
                                int characterID = GetCharacterID(placeToFill.presentCharacters[j]);
                                if (characterID >= 0 && characterToRoleAssignments[characterID] >= 0)
                                {
                                    for (int k = locations[i].presentCharacters.Length - 1; k >= 0; k--)
                                    {
                                        if (locations[i].presentCharacters[k] == characters[characterToRoleAssignments[characterID]])
                                        {
                                            hasNarreme = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!hasNarreme) { score++; }

                        }
                        if (score.x < highScore) { highScore = score.x; highID = i; }
                    }
                    break;
            }

            return highID;
        }
        public virtual bool SetLocationToPlace(int locationID, int placeID, ReplaceType replacement = ReplaceType.None)
        {
            UnityLocation currentLocationInPlace = null;
            if (locationToPlaceAssignments[placeID] >= 0)
            {
                currentLocationInPlace = locations[locationToPlaceAssignments[placeID]];
                
            }

            switch (replacement)
            {
                case ReplaceType.Symbolic:

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].title == places[placeID].name ||
                                characters[i].relationships[j].subject == places[placeID])
                            {
                                characters[i].relationships[j].subject = locations[locationID];
                            }
                        }
                    }

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalTitles[j] == places[placeID].name ||
                                events[i].conditionalNarremes[j] == places[placeID])
                            {
                                events[i].conditionalNarremes[j] = locations[locationID];
                            }
                        }
                    }
                    break;

                case ReplaceType.Literal:

                    if (currentLocationInPlace == null) { break; }

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        if (characters[i].currentLocation == currentLocationInPlace)
                        {
                            characters[i].currentLocation = locations[locationID];
                        }

                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].subject == currentLocationInPlace)
                            {
                                characters[i].relationships[j].subject = locations[locationID];
                            }
                        }
                    }

                    for (int i = objects.Length - 1; i >= 0; i--)
                    {
                        if (objects[i].currentLocation == currentLocationInPlace)
                        {
                            objects[i].currentLocation = locations[locationID];
                        }
                    }

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalNarremes[j] == currentLocationInPlace)
                            {
                                events[i].conditionalNarremes[j] = locations[locationID];
                            }
                        }
                    }

                    break;

                case ReplaceType.Thorough:
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        if ((currentLocationInPlace != null && characters[i].currentLocation == currentLocationInPlace) ||
                            characters[i].currentLocation == places[placeID])
                        {
                            characters[i].currentLocation = locations[locationID];
                        }

                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if ((currentLocationInPlace != null && characters[i].relationships[j].subject == currentLocationInPlace) ||
                                characters[i].relationships[j].title == places[placeID].name ||
                                characters[i].relationships[j].subject == places[placeID])
                            {
                                characters[i].relationships[j].subject = locations[locationID];
                            }
                        }
                    }

                    for (int i = objects.Length - 1; i >= 0; i--)
                    {
                        if ((currentLocationInPlace != null && objects[i].currentLocation == currentLocationInPlace) ||
                            objects[i].currentLocation == places[placeID])
                        {
                            objects[i].currentLocation = locations[locationID];
                        }
                    }


                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if ((currentLocationInPlace != null && events[i].conditionalNarremes[j] == currentLocationInPlace) ||
                                events[i].conditionalTitles[j] == places[placeID].name ||
                                events[i].conditionalNarremes[j] == places[placeID])
                            {
                                events[i].conditionalNarremes[j] = locations[locationID];
                            }
                        }
                    }
                    break;
            }

            locationToPlaceAssignments[placeID] = locationID;
            OnSetLocationToPlace?.Invoke(locationID, placeID);
            return true;
        }

        public virtual void RemoveLocationFromPlace(int placeID) 
        {
            if ( locationToPlaceAssignments[placeID] < 0 ) { return; }
            
            int locationID = locationToPlaceAssignments[placeID];

            UnityLocation currentLocationInPlace = locations[locationToPlaceAssignments[placeID]];
            
            for (int i = characters.Length - 1; i >= 0; i--)
            {
                
                for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                {
                    if (characters[i].relationships[j].subject == currentLocationInPlace &&
                        characters[i].relationships[j].title == places[placeID].name)
                    {
                        characters[i].relationships[j].subject = null;
                    }
                }
            }


            for (int i = events.Length - 1; i >= 0; i--)
            {
                for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                {
                    if (events[i].conditionalNarremes[j] == currentLocationInPlace &&
                        events[i].conditionalTitles[j] == places[placeID].name)
                    {
                        events[i].conditionalNarremes[j] = null;
                    }
                }
            }
            locationToPlaceAssignments[placeID] = -1;
            OnRemoveLocationFromPlace?.Invoke(locationID, placeID);
        }

        public virtual int GetLocationInPlace(int placeID) { return locationToPlaceAssignments[placeID]; }
        public virtual int[] GetPlacesForLocation(int locationID)
        {
            List<int> placeIDs = new List<int>();
            for (int i = 0; i < places.Length; i++)
            {
                if (locationToPlaceAssignments[i] == locationID)
                {
                    placeIDs.Add(i);
                }
            }
            return placeIDs.ToArray();
        }

        #endregion

        #region Objects and Symbols

        public UnityProp[] objects = new UnityProp[0],
                        symbols = new UnityProp[0];

        public int[] objectToSymbolAssignments = new int[0];


        public virtual event Action<IProp> OnAddObject, OnRemoveObject,
                                                OnAddSymbol, OnRemoveSymbol;

        public virtual event Action<int, int> OnSetObjectToSymbol,
                                              OnRemoveObjectFromSymbol;

        public virtual int AddObject<T>(T newObject) where T : IProp
        {
            if (newObject is UnityProp obj)
            {
                return AddObject(obj, -1);
            }
            return -1;
        }
        public virtual int AddObject(UnityProp newObject, int id = -1)
        {
            int entryID = GetObjectID(newObject);
            if (entryID < 0)
            {
                List<UnityProp> newObjects = new List<UnityProp>(objects);

                if (id < 0 || id >= objects.Length)
                {
                    entryID = newObjects.Count;
                    newObjects.Add(newObject);
                }
                else
                {
                    newObjects.Insert(id, newObject);
                    entryID = id;

                    if (id < objects.Length - 1)
                    {
                        for (int i = objectToSymbolAssignments.Length - 1; i >= 0; i--)
                        {
                            if (objectToSymbolAssignments[i] >= id)
                            {
                                objectToSymbolAssignments[i]++;
                            }
                        }
                    }
                }

                objects = newObjects.ToArray();
                OnAddObject?.Invoke(newObject);

            }
            return entryID;
        }

        public virtual int GetObjectID(UnityProp obj)
        {
            for (int i = 0; i < objects.Length; i++)
            {
                if (objects[i] == obj) { return i; }
            }

            return -1;
        }

        public virtual void RemoveObject<T>(T toRemove) where T : IProp
        {
            if (toRemove is UnityProp obj)
            {
                int id = GetObjectID(obj);
                if (id >= 0) { RemoveObject(id); }
            }
        }
        public virtual void RemoveObject(int toRemove)
        {
            if (toRemove >= 0 && toRemove < objects.Length)
            {
                for (int i = objectToSymbolAssignments.Length - 1; i >= 0; i--)
                {
                    if (objectToSymbolAssignments[i] > toRemove)
                    { objectToSymbolAssignments[i]--; }
                    else if (objectToSymbolAssignments[i] == toRemove) { RemoveObjectFromSymbol(i); }
                }
                UnityProp obj = objects[toRemove];
                List<UnityProp> newObjects = new List<UnityProp>(objects);
                newObjects.RemoveAt(toRemove);
                objects = newObjects.ToArray();
                OnRemoveObject?.Invoke(obj);
            }
        }

        public virtual int AddSymbol<T>(T newSymbol) where T : IProp
        {
            if (newSymbol is UnityProp symbol)
            {
                return AddSymbol(symbol, -1);
            }
            return -1;
        }
        public virtual int AddSymbol(UnityProp newSymbol, int id = -1)
        {
            int entryID = GetSymbolID(newSymbol);
            if (entryID < 0)
            {
                List<UnityProp> newSymbols = new List<UnityProp>(symbols);
                List<int> newObjectToSymbolAssignments = new List<int>(objectToSymbolAssignments);

                if (id < 0 || id >= symbols.Length)
                {
                    entryID = newSymbols.Count;
                    
                    newSymbols.Add(newSymbol);
                    newObjectToSymbolAssignments.Add(-1);
                }
                else
                {
                    newSymbols.Insert(id, newSymbol);
                    newObjectToSymbolAssignments.Insert(id, -1);

                    entryID = id;
                }

                symbols = newSymbols.ToArray();
                objectToSymbolAssignments = newObjectToSymbolAssignments.ToArray();
                
                OnAddSymbol?.Invoke(newSymbol);

                return symbols.Length - 1;
            }
            return entryID;
        }

        public virtual int GetSymbolID(UnityProp symbol)
        {
            for (int i = 0; i < symbols.Length; i++)
            {
                if (symbols[i] == symbol) { return i; }
            }
            return -1;
        }

        public virtual void RemoveSymbol<T>(T toRemove) where T : IProp
        {
            if (toRemove is UnityProp symbol)
            {
                int id = GetSymbolID(symbol);
                if (id >= 0) { RemoveSymbol(id); }
            }
        }
        public virtual void RemoveSymbol(int toRemove)
        {
            if (toRemove >= 0 && toRemove < symbols.Length)
            {
                UnityProp symbol = symbols[toRemove];
                RemoveObjectFromSymbol(toRemove);

                List<UnityProp> newSymbols = new List<UnityProp>(symbols);
                newSymbols.RemoveAt(toRemove);
                symbols = newSymbols.ToArray();

                List<int> newObjectToSymbolAssignments = new List<int>(objectToSymbolAssignments);
                newObjectToSymbolAssignments.RemoveAt(toRemove);
                objectToSymbolAssignments = newObjectToSymbolAssignments.ToArray();

                OnRemoveSymbol?.Invoke(symbol);
            }
        }

        public virtual int GetBestObjectFitForSymbol(int symbolID, int[] masked, BestFit bestFit = BestFit.Basic) 
        {
            if (symbolID < 0 || symbolID >= symbols.Length) { return -1; }

            int highScore = int.MaxValue, highID = -1;
            Int score = new Int(0);

            UnityProp symbolToFill = symbols[symbolID];
            UnityProp currentObjectInSymbol;
            int currentObjectID = objectToSymbolAssignments[symbolID];

            if (currentObjectID >= 0) { currentObjectInSymbol = objects[currentObjectID]; }

            switch (bestFit)
            {
                case BestFit.Basic:
                    for (int i = objects.Length - 1; i >= 0; i--)
                    {
                        //Removing this check. If you don't want to
                        //include the current obj, you can get the ID of the
                        //current obj in the symbol and put it in the mask list.
                        //if (i == currentObjectID) { continue; }

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        if (symbolToFill.semantics.CompareSemanticAffinity<Int>(objects[i].semantics, out score))
                        {
                            if (score.x < highScore)
                            {
                                highScore = score.x;
                                highID = i;
                            }
                        }
                    }
                    break;

                case BestFit.Symbolic:
                    for (int i = objects.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }

                        symbolToFill.semantics.CompareSemanticAffinity<Int>(objects[i].semantics, out score);

                        if (symbolToFill.currentPossessor != null)
                        {
                            UnityCharacter originalPossessor = symbolToFill.currentPossessor;
                            int roleID = GetRoleID(originalPossessor);

                            if (characterToRoleAssignments[roleID] >= 0 &&
                                characters[characterToRoleAssignments[roleID]] != objects[i].currentPossessor)
                            {
                                score++;
                            }
                        }
                        else if (objects[i].currentPossessor != null)
                        {
                            score++;
                        }

                        if (symbolToFill.currentLocation != null)
                        {
                            UnityLocation originalPlace = symbolToFill.currentLocation;
                            int placeID = GetPlaceID(originalPlace);

                            if (locationToPlaceAssignments[placeID] >= 0 &&
                                locations[locationToPlaceAssignments[placeID]] != objects[i].currentLocation)
                            {
                                score++;
                            }
                        }
                        else if (objects[i].currentLocation != null)
                        {
                            score++;
                        }

                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Literal:
                    for (int i = objects.Length - 1; i >= 0; i--)
                    {
                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        symbolToFill.semantics.CompareSemanticAffinity<Int>(objects[i].semantics, out score);

                        if (symbolToFill.currentLocation != objects[i].currentLocation)
                        { score++; }

                        if (symbolToFill.currentPossessor != objects[i].currentPossessor)
                        { score++; }

                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Thorough:
                    for (int i = objects.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        symbolToFill.semantics.CompareSemanticAffinity<Int>(objects[i].semantics, out score);

                        if (symbolToFill.currentLocation != null)
                        {
                            if (symbolToFill.currentLocation != objects[i].currentLocation)
                            {
                                int locID = GetPlaceID(symbolToFill.currentLocation);

                                if (locID < 0 || locations[locationToPlaceAssignments[locID]] != objects[i].currentLocation)
                                {
                                    score++;
                                }
                            }
                        }
                        else if (objects[i].currentLocation != null)
                        {
                            score++;
                        }

                        if (symbolToFill.currentPossessor != null)
                        {
                            if (symbolToFill.currentPossessor != objects[i].currentPossessor)
                            {
                                int roleID = GetRoleID(symbolToFill.currentPossessor);

                                if (roleID < 0 || characters[characterToRoleAssignments[roleID]] != objects[i].currentPossessor)
                                {
                                    score++;
                                }
                            }
                        }
                        else if (objects[i].currentPossessor != null)
                        {
                            score++;
                        }

                        if (score.x < highScore) { highScore = score.x; highID = i; }
                    }
                    break;
            }

            return highID;
        }
        
        public virtual bool SetObjectToSymbol(int objectID, int symbolID, ReplaceType replacement = ReplaceType.None) 
        {
            UnityProp currentObjectInSymbol = null;
            if (objectToSymbolAssignments[symbolID] >= 0)
            {
                currentObjectInSymbol = objects[objectToSymbolAssignments[symbolID]];
            }

            switch (replacement)
            {
                case ReplaceType.Symbolic:

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].title == symbols[symbolID].name ||
                                characters[i].relationships[j].subject == symbols[symbolID])
                            {
                                characters[i].relationships[j].subject = objects[objectID];
                            }
                        }
                    }

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalTitles[j] == symbols[symbolID].name ||
                                events[i].conditionalNarremes[j] == symbols[symbolID])
                            {
                                events[i].conditionalNarremes[j] = objects[objectID];
                            }
                        }
                    }
                    break;

                case ReplaceType.Literal:
                    if (currentObjectInSymbol == null) { break; }

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for(int j = characters[i].possessions.Length-1; j >=0; j--)
                        {
                            if(characters[i].possessions[j] == currentObjectInSymbol)
                            {
                                characters[i].possessions[j] = objects[objectID];
                            }
                        }
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].subject == currentObjectInSymbol)
                            {
                                characters[i].relationships[j].subject = characters[objectID];
                            }
                        }
                    }

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalNarremes[j] == currentObjectInSymbol)
                            {
                                events[i].conditionalNarremes[j] = characters[objectID];
                            }
                        }
                    }
                    break;
                case ReplaceType.Thorough:
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].possessions.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].possessions[j] == currentObjectInSymbol)
                            {
                                characters[i].possessions[j] = objects[objectID];
                            }
                        }
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if ((currentObjectInSymbol != null && characters[i].relationships[j].subject == currentObjectInSymbol) ||
                                characters[i].relationships[j].subject == symbols[symbolID] ||
                                characters[i].relationships[j].title == symbols[symbolID].name)
                            {
                                characters[i].relationships[j].subject = objects[objectID];
                            }
                        }
                    }


                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if ((currentObjectInSymbol != null && events[i].conditionalNarremes[j] == currentObjectInSymbol) ||
                                events[i].conditionalNarremes[j] == symbols[symbolID] ||
                                events[i].conditionalTitles[j] == symbols[symbolID].name)
                            {
                                events[i].conditionalNarremes[j] = objects[objectID];
                            }
                        }
                    }
                    break;
            }

            objectToSymbolAssignments[symbolID] = objectID;

            OnSetObjectToSymbol?.Invoke(objectID, symbolID);

            return true;
        }
        
        public virtual void RemoveObjectFromSymbol(int symbolID) 
        {
            if (objectToSymbolAssignments[symbolID] < 0) { return; }
            
            int objectID = objectToSymbolAssignments[symbolID];

            UnityProp currentObjectInSymbol = objects[objectToSymbolAssignments[symbolID]];

            for (int i =characters.Length - 1; i >= 0; i--)
            {
                for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                {
                    if ((characters[i].relationships[j].subject == currentObjectInSymbol) &&
                        characters[i].relationships[j].title == symbols[symbolID].name)
                    {
                        characters[i].relationships[j].subject = null;
                    }
                }
            }

            for (int i = events.Length - 1; i >= 0; i--)
            {
                for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                {
                    if ((events[i].conditionalNarremes[j] == currentObjectInSymbol) &&
                        events[i].conditionalTitles[j] == symbols[symbolID].name)
                    {
                        events[i].conditionalNarremes[j] = null;
                    }
                }
            }
            objectToSymbolAssignments[symbolID] = -1;
            OnRemoveObjectFromSymbol?.Invoke(objectID, symbolID);
        }

        public virtual int GetSymbolizedObject(int symbolID) { return objectToSymbolAssignments[symbolID]; }
        public virtual int[] GetObjectifiedSymbols(int objectID)
        {
            List<int> symbolIDs = new List<int>();
            for (int i = 0; i < symbols.Length; i++)
            {
                if (objectToSymbolAssignments[i] == objectID)
                {
                    symbolIDs.Add(i);
                }
            }
            return symbolIDs.ToArray();
        }

        #endregion

        #region Plots and Stories

        public UnityPlot[] plots = new UnityPlot[0],
                      stories = new UnityPlot[0];

        public int[] plotToStoryAssignments = new int[0];


        public virtual event Action<IPlot> OnAddPlot, OnRemovePlot,
                                                OnAddStory, OnRemoveStory;

        public virtual event Action<int, int> OnSetPlotToStory,
                                              OnRemovePlotFromStory;

        public virtual int AddPlot<T>(T newPlot) where T : IPlot
        {
            if (newPlot is UnityPlot plot)
            {
                return AddPlot(plot, -1);
            }
            return -1;
        }
        public virtual int AddPlot(UnityPlot newPlot, int id = -1)
        {
            int entryID = GetPlotID(newPlot);
            if (entryID < 0)
            {
                List<UnityPlot> newPlots = new List<UnityPlot>(plots);

                if (id < 0 || id >= plots.Length)
                {
                    entryID = newPlots.Count;
                    newPlots.Add(newPlot);
                }
                else
                {
                    newPlots.Insert(id, newPlot);
                    entryID = id;
                    if (id < plots.Length - 1)
                    {
                        for (int i = plotToStoryAssignments.Length - 1; i >= 0; i--)
                        {
                            if (plotToStoryAssignments[i] >= id)
                            {
                                plotToStoryAssignments[i]++;
                            }
                        }
                    }
                }

                plots = newPlots.ToArray();
                OnAddPlot?.Invoke(newPlot);

            }
            return entryID;
        }

        public virtual int GetPlotID(UnityPlot plot)
        {
            for (int i = 0; i < plots.Length; i++)
            {
                if (plots[i] == plot) { return i; }
            }

            return -1;
        }

        public virtual void RemovePlot<T>(T toRemove) where T : IPlot
        {
            if (toRemove is UnityPlot plot)
            {
                int id = GetPlotID(plot);
                if (id >= 0) { RemovePlot(id); }
            }
        }
        public virtual void RemovePlot(int toRemove)
        {
            if (toRemove >= 0 && toRemove < plots.Length)
            {
                for (int i = plotToStoryAssignments.Length - 1; i >= 0; i--)
                {
                    if (plotToStoryAssignments[i] > toRemove)
                    { plotToStoryAssignments[i]--; }
                    else if (plotToStoryAssignments[i] == toRemove) { RemovePlotFromStory(i); }
                }
                UnityPlot plot = plots[toRemove];
                List<UnityPlot> newPlots = new List<UnityPlot>(plots);
                newPlots.RemoveAt(toRemove);
                plots = newPlots.ToArray();
                OnRemovePlot?.Invoke(plot);
            }
        }


        public virtual int AddStory<T>(T newStory) where T : IPlot
        {
            if (newStory is UnityPlot story)
            {
                return AddStory(story, -1);
            }
            return -1;
        }
        public virtual int AddStory(UnityPlot newStory, int id = -1)
        {
            int entryID = GetStoryID(newStory);
            if (entryID < 0)
            {
                List<UnityPlot> newStories = new List<UnityPlot>(stories);
                List<int> newPlotToStoryAssignments = new List<int>(plotToStoryAssignments);

                if (id < 0 || id >= stories.Length)
                {
                    entryID = newStories.Count;

                    newStories.Add(newStory);
                    newPlotToStoryAssignments.Add(-1);
                }
                else
                {
                    newStories.Insert(id, newStory);
                    newPlotToStoryAssignments.Insert(id, -1);

                    entryID = id;
                }

                stories = newStories.ToArray();
                plotToStoryAssignments = newPlotToStoryAssignments.ToArray();

                OnAddStory?.Invoke(newStory);

                return stories.Length - 1;
            }
            return entryID;
        }

        public virtual int GetStoryID(UnityPlot story)
        {
            for (int i = 0; i < stories.Length; i++)
            {
                if (stories[i] == story) { return i; }
            }
            return -1;
        }

        public virtual void RemoveStory<T>(T toRemove) where T : IPlot
        {
            if (toRemove is UnityPlot story)
            {
                int id = GetStoryID(story);
                if (id >= 0) { RemoveStory(id); }
            }
        }
        public virtual void RemoveStory(int toRemove)
        {
            if (toRemove >= 0 && toRemove < stories.Length)
            {
                UnityPlot story = stories[toRemove];
                RemovePlotFromStory(toRemove);

                List<UnityPlot> newStories = new List<UnityPlot>(stories);
                newStories.RemoveAt(toRemove);
                stories = newStories.ToArray();

                List<int> newPlotToStoryAssignments = new List<int>(plotToStoryAssignments);
                newPlotToStoryAssignments.RemoveAt(toRemove);
                plotToStoryAssignments = newPlotToStoryAssignments.ToArray();

                OnRemoveStory?.Invoke(story);
            }
        }


        public virtual int GetBestPlotFitForStory(int storyID, int[] masked, BestFit bestFit = BestFit.Basic) 
        {
            if (storyID < 0 || storyID >= stories.Length) { return -1; }

            int highScore = int.MaxValue, highID = -1;
            Int score = new Int(0);

            UnityPlot storyToFill = stories[storyID];
            UnityPlot currentPlotInStory;
            int currentPlotID = plotToStoryAssignments[storyID];

            if (currentPlotID >= 0) { currentPlotInStory = plots[currentPlotID]; }

            switch (bestFit)
            {
                case BestFit.Basic:
                    for (int i = plots.Length - 1; i >= 0; i--)
                    {
                        //Removing this check. If you don't want to
                        //include the current plot, you can get the ID of the
                        //current plot in the story and put it in the mask list.
                        //if (i == currentPlotID) { continue; }

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        if (storyToFill.semantics.CompareSemanticAffinity<Int>(plots[i].semantics, out score))
                        {
                            if (score.x < highScore)
                            {
                                highScore = score.x;
                                highID = i;
                            }
                        }
                    }
                    break;

                case BestFit.Symbolic:
                    for (int i = plots.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }

                        storyToFill.semantics.CompareSemanticAffinity<Int>(plots[i].semantics, out score);

                        for (int j = storyToFill.plotIncidents.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;

                            UnityIncident storyEvent = storyToFill.plotIncidents[j].plotIncident;
                            int posID = GetBeatID(storyEvent);

                            if (eventToBeatAssignments[posID] >= 0)
                            {
                                UnityIncident currentEvent = events[eventToBeatAssignments[posID]];

                                for (int k = plots[i].plotIncidents.Length - 1; k >= 0; k--)
                                {
                                    if (plots[i].plotIncidents[k].plotIncident == currentEvent)
                                    {
                                        hasNarreme = true;
                                        break;
                                    }
                                }
                                if (!hasNarreme) { score++; }
                            }
                        }


                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Literal:
                    for (int i = plots.Length - 1; i >= 0; i--)
                    {
                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        storyToFill.semantics.CompareSemanticAffinity<Int>(plots[i].semantics, out score);

                        

                        for (int j = storyToFill.plotIncidents.Length - 1; j >= 0; j--)
                        {
                            bool hasNarreme = false;
                            for (int k = plots[i].plotIncidents.Length - 1; k >= 0; k--)
                            {
                                if (plots[i].plotIncidents[k] == storyToFill.plotIncidents[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }
                            }
                            if (!hasNarreme) { score++; }
                        }

                        

                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Thorough:
                    for (int i = plots.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        storyToFill.semantics.CompareSemanticAffinity<Int>(plots[i].semantics, out score);

                        

                        for (int j = storyToFill.plotIncidents.Length - 1; j >= 0; j--)
                        {

                            bool hasNarreme = false;
                            for (int k = plots[i].plotIncidents.Length - 1; k >= 0; k--)
                            {
                                if (plots[i].plotIncidents[k] == storyToFill.plotIncidents[j])
                                {
                                    hasNarreme = true;
                                    break;
                                }

                            }
                            if (!hasNarreme)
                            {
                                int posID = GetBeatID(storyToFill.plotIncidents[j].plotIncident);
                                if (posID >= 0 && eventToBeatAssignments[posID] >= 0)
                                {
                                    for (int k = plots[i].plotIncidents.Length - 1; k >= 0; k--)
                                    {
                                        if (plots[i].plotIncidents[k].plotIncident == events[eventToBeatAssignments[posID]])
                                        {
                                            hasNarreme = true;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (!hasNarreme) { score++; }

                        }

                        if (score.x < highScore) { highScore = score.x; highID = i; }
                    }
                    break;
            }

            return highID;
        }
        public virtual bool SetPlotToStory(int plotID, int storyID, ReplaceType replacement = ReplaceType.None) 
        {
            UnityPlot originalStory = null;
            if (plotToStoryAssignments[storyID] >= 0)
            {
                originalStory = plots[plotToStoryAssignments[storyID]];
            }

            switch (replacement)
            {
                case ReplaceType.Symbolic:

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].title == stories[storyID].name ||
                                characters[i].relationships[j].subject == stories[storyID])
                            {
                                characters[i].relationships[j].subject = plots[plotID];
                            }
                        }
                    }

                   
                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalTitles[j] == stories[storyID].name ||
                                events[i].conditionalNarremes[j] == stories[storyID])
                            {
                                events[i].conditionalNarremes[j] = plots[plotID];
                            }
                        }
                    }

                    break;

                case ReplaceType.Literal:
                    if (originalStory == null) { break; }
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].subject == originalStory)
                            {
                                characters[i].relationships[j].subject = plots[plotID];
                            }
                        }

                        for (int j = characters[i].goals.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].goals[j] == originalStory)
                            {
                                characters[i].goals[j] = plots[plotID];
                            }
                        }
                    }

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalNarremes[j] == originalStory)
                            {
                                events[i].conditionalNarremes[j] = plots[plotID];
                            }
                        }
                    }
                    break;

                case ReplaceType.Thorough:
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if ((originalStory != null && characters[i].relationships[j].subject == originalStory) ||
                                characters[i].relationships[j].title == stories[storyID].name ||
                                characters[i].relationships[j].subject == stories[storyID])
                            {
                                characters[i].relationships[j].subject = plots[plotID];
                            }
                        }

                        for (int j = characters[i].goals.Length - 1; j >= 0; j--)
                        {
                            if ((originalStory != null && characters[i].goals[j] == originalStory) ||
                                characters[i].goals[j] == stories[storyID])
                            {
                                characters[i].goals[j] = plots[plotID];
                            }
                        }
                    }

                    

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if ((originalStory != null && events[i].conditionalNarremes[j] == originalStory) ||
                                events[i].conditionalNarremes[j] == stories[storyID] ||
                                events[i].conditionalTitles[j] == stories[storyID].name)
                            {
                                events[i].conditionalNarremes[j] = plots[plotID];
                            }
                        }
                    }
                    break;
            }

            plotToStoryAssignments[storyID] = plotID;
            OnSetPlotToStory?.Invoke(plotID, storyID);
            return true;
        }
        public virtual void RemovePlotFromStory(int storyID) 
        {
            if (plotToStoryAssignments[storyID] < 0) { return; }
            
            int plotID = plotToStoryAssignments[storyID];

            UnityPlot currentPlotInStory = plots[plotToStoryAssignments[storyID]];

            for (int i = characters.Length - 1; i >= 0; i--)
            {
                for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                {
                    if ((characters[i].relationships[j].subject == currentPlotInStory) &&
                        characters[i].relationships[j].title == stories[storyID].name)
                    {
                        characters[i].relationships[j].subject = null;
                    }
                }
            }

            for (int i = events.Length - 1; i >= 0; i--)
            {
                for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                {
                    if ((events[i].conditionalNarremes[j] == currentPlotInStory) &&
                        events[i].conditionalTitles[j] == stories[storyID].name)
                    {
                        events[i].conditionalNarremes[j] = null;
                    }
                }
            }
            plotToStoryAssignments[storyID] = -1;
            OnRemovePlotFromStory?.Invoke(plotID, storyID);
        }

        public virtual int GetPlotOfStory(int storyID) { return plotToStoryAssignments[storyID]; }
        public virtual int[] GetStoriesInPlot(int plotID)
        {
            List<int> storyIDs = new List<int>();
            for (int i = 0; i < stories.Length; i++)
            {
                if (plotToStoryAssignments[i] == plotID)
                {
                    storyIDs.Add(i);
                }
            }
            return storyIDs.ToArray();
        }

        #endregion

        #region Events and Beats

        public UnityIncident[] events = new UnityIncident[0],
                       beats = new UnityIncident[0];

        public int[] eventToBeatAssignments = new int[0];


        public virtual event Action<IIncident> OnAddEvent, OnRemoveEvent,
                                                OnAddBeat, OnRemoveBeat;

        public virtual event Action<int, int> OnSetEventToBeat,
                                              OnRemoveEventFromBeat;

        public virtual int AddEvent<T>(T newEvent) where T : IIncident
        {
            if (newEvent is UnityIncident evnt)
            {
                return AddEvent(evnt, -1);
            }
            return -1;
        }
        public virtual int AddEvent(UnityIncident newEvent, int id = -1)
        {
            int entryID = GetEventID(newEvent);
            if (entryID < 0)
            {
                List<UnityIncident> newEvents = new List<UnityIncident>(events);

                if (id < 0 || id >= events.Length)
                {
                    entryID = newEvents.Count;
                    newEvents.Add(newEvent);
                }
                else
                {
                    newEvents.Insert(id, newEvent);
                    entryID = id;

                    if (id < events.Length - 1)
                    {
                        for (int i = eventToBeatAssignments.Length - 1; i >= 0; i--)
                        {
                            if (eventToBeatAssignments[i] >= id)
                            {
                                eventToBeatAssignments[i]++;
                            }
                        }
                    }
                }

                events = newEvents.ToArray();
                OnAddEvent?.Invoke(newEvent);
            }
            return entryID;
        }

        public virtual int GetEventID(UnityIncident evnt)
        {
            for (int i = 0; i < events.Length; i++)
            {
                if (events[i] == evnt) { return i; }
            }

            return -1;
        }

        public virtual void RemoveEvent<T>(T toRemove) where T : IIncident
        {
            if (toRemove is UnityIncident evnt)
            {
                int id = GetEventID(evnt);
                if (id >= 0) { RemoveEvent(id); }
            }
        }
        public virtual void RemoveEvent(int toRemove)
        {
            if (toRemove >= 0 && toRemove < events.Length)
            {
                for (int i = eventToBeatAssignments.Length - 1; i >= 0; i--)
                {
                    if (eventToBeatAssignments[i] > toRemove)
                    { eventToBeatAssignments[i]--; }
                    else if (eventToBeatAssignments[i] == toRemove) { RemoveEventFromBeat(i); }
                }
                UnityIncident evnt = events[toRemove];
                List<UnityIncident> newEvents = new List<UnityIncident>(events);
                newEvents.RemoveAt(toRemove);
                events = newEvents.ToArray();
                OnRemoveEvent?.Invoke(evnt);
            }
        }


        public virtual int AddBeat<T>(T newBeat) where T : IIncident
        {
            if (newBeat is UnityIncident beat)
            {
                return AddBeat(beat, -1);
            }
            return -1;
        }
        public virtual int AddBeat(UnityIncident newBeat, int id = -1)
        {
            int entryID = GetBeatID(newBeat);
            if (entryID < 0)
            {
                List<UnityIncident> newBeats = new List<UnityIncident>(beats);
                List<int> newEventToBeatAssignments = new List<int>(eventToBeatAssignments);

                if (id < 0 || id >= beats.Length)
                {
                    entryID = newBeats.Count;
                    newBeats.Add(newBeat);
                    newEventToBeatAssignments.Add(-1);
                }
                else
                {
                    newBeats.Insert(id, newBeat);
                    newEventToBeatAssignments.Insert(id, -1);
                    entryID = id;
                }

                beats = newBeats.ToArray();
                eventToBeatAssignments = newEventToBeatAssignments.ToArray();

                OnAddBeat?.Invoke(newBeat);

                return beats.Length - 1;
            }
            return entryID;
        }

        public virtual int GetBeatID(UnityIncident beat)
        {
            for (int i = 0; i < beats.Length; i++)
            {
                if (beats[i] == beat) { return i; }
            }
            return -1;
        }

        public virtual void RemoveBeat<T>(T toRemove) where T : IIncident
        {
            if (toRemove is UnityIncident beat)
            {
                int id = GetBeatID(beat);
                if (id >= 0) { RemoveBeat(id); }
            }
        }
        public virtual void RemoveBeat(int toRemove)
        {
            if (toRemove >= 0 && toRemove < beats.Length)
            {
                UnityIncident beat = beats[toRemove];
                RemoveEventFromBeat(toRemove);

                List<UnityIncident> newBeats = new List<UnityIncident>(beats);
                newBeats.RemoveAt(toRemove);
                beats = newBeats.ToArray();

                List<int> newEventToBeatAssignments = new List<int>(eventToBeatAssignments);
                newEventToBeatAssignments.RemoveAt(toRemove);
                eventToBeatAssignments = newEventToBeatAssignments.ToArray();

                OnRemoveBeat?.Invoke(beat);
            }
        }


        public virtual int GetBestEventFitForBeat(int beatID, int[] masked, BestFit bestFit = BestFit.Basic) 
        {
            if (beatID < 0 || beatID >= beats.Length) { return -1; }

            int highScore = int.MaxValue, highID = -1;
            Int score = new Int(0);

            UnityIncident beatToFill = beats[beatID];
            UnityIncident currentEventInBeat;
            int currentEventID = eventToBeatAssignments[beatID];

            if (currentEventID >= 0) { currentEventInBeat = events[currentEventID]; }

            switch (bestFit)
            {
                case BestFit.Basic:
                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        //Removing this check. If you don't want to
                        //include the current character, you can get the ID of the
                        //current character in the beat and put it in the mask list.
                        //if (i == currentEventID) { continue; }

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }


                        if (beatToFill.semantics.CompareSemanticAffinity<Int>(events[i].semantics, out score))
                        {
                            if (score.x < highScore)
                            {
                                highScore = score.x;
                                highID = i;
                            }
                        }
                    }
                    break;

                case BestFit.Symbolic:
                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }

                        beatToFill.semantics.CompareSemanticAffinity<Int>(events[i].semantics, out score);

                        for (int j = beatToFill.conditionalNarremes.Length - 1; j >= 0; j--)
                        {
                            bool hasTitle = false, hasNarreme = false;
                            for (int k = events[i].conditionalNarremes.Length - 1; k >= 0; k--)
                            {
                                if (events[i].conditionalTitles[k] == beatToFill.conditionalTitles[j])
                                {
                                    hasTitle = true;
                                    if (beatToFill.conditionalNarremes[j] != null)
                                    {
                                        UnityNarreme beatRelation = GetNarremeFromSymbolName(beatToFill.conditionalTitles[j]);
                                        if (beatRelation != null && beatRelation == events[i].conditionalNarremes[j])
                                        {
                                            hasNarreme = true;
                                        }
                                    }
                                    else if (events[i].conditionalNarremes[k] == null)
                                    {
                                        hasNarreme = true;
                                    }
                                    break;
                                }
                            }
                            if (!hasTitle) { score += 2; }
                            else if (!hasNarreme) { score++; }
                        }

                        for (int j = beatToFill.narremeConditions.Length - 1; j >= 0; j--)
                        {
                            bool foundCondition = false;

                            NarremeCondition narConToFill = beatToFill.narremeConditions[j];

                            if(narConToFill.conditionSubject < 0) { continue; }

                            for (int k = events[i].narremeConditions.Length - 1; k >= 0; k--)
                            {
                                NarremeCondition eventNarCon = events[i].narremeConditions[k];

                                if ( eventNarCon.conditionSubject <0 || narConToFill.condition != eventNarCon.condition) { continue; }

                                if (beatToFill.conditionalTitles[narConToFill.conditionSubject] ==
                                    events[i].conditionalTitles[eventNarCon.conditionSubject] &&
                                    beatToFill.conditionalTitles[narConToFill.conditionObject] ==
                                    events[i].conditionalTitles[eventNarCon.conditionObject])
                                {
                                    foundCondition = true;
                                    break;
                                }
                            }

                            if (!foundCondition) { score++; }
                        }

                        for (int j = beatToFill.semanticConditions.Length - 1; j >= 0; j--)
                        {
                            bool foundCondition = false;
                            
                            SemanticAffinityCondition semConToFill = beatToFill.semanticConditions[j];
                            if(semConToFill.conditionSubject < 0) { continue; }

                            for (int k = events[i].semanticConditions.Length-1; k >=0; k--)
                            {
                                SemanticAffinityCondition eventSemCon = events[i].semanticConditions[k];

                                if( eventSemCon.conditionSubject < 0 ||
                                    semConToFill.affinityComparison != eventSemCon.affinityComparison ||
                                    semConToFill.subjectSememe != eventSemCon.subjectSememe ||
                                    semConToFill.objectSememe != eventSemCon.objectSememe) 
                                { continue; }
                                

                                if (beatToFill.conditionalTitles[semConToFill.conditionSubject] ==
                                    events[i].conditionalTitles[eventSemCon.conditionSubject] &&
                                    beatToFill.conditionalTitles[semConToFill.conditionObject] ==
                                    events[i].conditionalTitles[eventSemCon.conditionObject]) 
                                { foundCondition = true; break; }
                            }

                            if (!foundCondition) { score++; }
                        }

                        for(int j = beatToFill.causalCondition.causes.Length - 1; j >= 0; j--)
                        {
                            bool foundCause = false;
                            if (beatToFill.causalCondition.causes[j] < 0) { continue; }
                            
                            for (int k = events[i].causalCondition.causes.Length - 1; k >=0; k--) 
                            {
                                if(events[i].causalCondition.causes[k] < 0) { continue; }

                                if(beatToFill.conditionalTitles[beatToFill.causalCondition.causes[j]] ==
                                    events[i].conditionalTitles[events[i].causalCondition.causes[k]])
                                { foundCause = true; break; }
                                
                                
                            }
                            if (!foundCause) { score++; }
                        }

                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Literal:
                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }

                        beatToFill.semantics.CompareSemanticAffinity<Int>(events[i].semantics, out score);

                        for (int j = beatToFill.conditionalNarremes.Length - 1; j >= 0; j--)
                        {
                            bool hasTitle = false, hasNarreme = false;
                            for (int k = events[i].conditionalNarremes.Length - 1; k >= 0; k--)
                            {
                                if (events[i].conditionalTitles[k] == beatToFill.conditionalTitles[j])
                                {
                                    hasTitle = true;
                                    if (beatToFill.conditionalNarremes[j] != null &&
                                        events[i].conditionalNarremes[k] == beatToFill.conditionalNarremes[j])
                                    {
                                        hasNarreme = true;
                                    }
                                    else if (beatToFill.conditionalNarremes[j] == null &&
                                        events[i].conditionalNarremes[k] == null)
                                    {
                                        hasNarreme = true;
                                    }
                                    break;
                                }
                            }
                            if (!hasTitle) { score += 2; }
                            else if (!hasNarreme) { score++; }
                        }

                        for (int j = beatToFill.narremeConditions.Length - 1; j >= 0; j--)
                        {
                            bool foundCondition = false;

                            NarremeCondition narConToFill = beatToFill.narremeConditions[j];
                            if (narConToFill.conditionSubject < 0) { continue; }

                            for (int k = events[i].narremeConditions.Length - 1; k >= 0; k--)
                            {
                                NarremeCondition eventNarCon = events[i].narremeConditions[k];

                                if(eventNarCon.conditionSubject < 0 ||
                                   narConToFill.condition != eventNarCon.condition ||
                                   beatToFill.conditionalTitles[narConToFill.conditionSubject] != 
                                   events[i].conditionalTitles[eventNarCon.conditionSubject])
                                { continue; }

                                UnityNarreme subLiteral, obLiteral, eventSub, eventOb;
                                
                                subLiteral = beatToFill.conditionalNarremes[narConToFill.conditionSubject].GetLiteralForm<UnityNarreme>();
                                eventSub = events[i].conditionalNarremes[eventNarCon.conditionSubject];

                                if (narConToFill.conditionObject < 0)
                                {
                                    if(eventNarCon.conditionObject < 0)
                                    {
                                        //Check just the subjects. If they're the same, 
                                        //this fits the condition.
                                        if(subLiteral == eventSub) { foundCondition = true; break; }
                                    }
                                    continue;
                                }
                                else if(eventNarCon.conditionObject < 0) { continue; }

                                if (beatToFill.conditionalTitles[narConToFill.conditionObject] !=
                                    events[i].conditionalTitles[eventNarCon.conditionObject])
                                { continue; }

                                obLiteral = beatToFill.conditionalNarremes[narConToFill.conditionObject].GetLiteralForm<UnityNarreme>();
                                eventOb = events[i].conditionalNarremes[eventNarCon.conditionObject];

                                if (eventSub == subLiteral && eventOb == obLiteral) 
                                { foundCondition = true; break; }
                            }
                            if (!foundCondition) { score++; }
                        }

                        for (int j = beatToFill.semanticConditions.Length - 1; j >= 0; j--)
                        {
                            bool foundCondition = false;
                            
                            SemanticAffinityCondition semConToFill = beatToFill.semanticConditions[j];
                            
                            if(semConToFill.conditionSubject < 0) { continue; }

                            for (int k = events[i].semanticConditions.Length - 1; k >= 0; k--)
                            {
                                SemanticAffinityCondition eventSemCon = events[i].semanticConditions[k];

                                if( eventSemCon.conditionSubject < 0 ||
                                    semConToFill.subjectSememe != eventSemCon.subjectSememe ||
                                    semConToFill.objectSememe != eventSemCon.objectSememe ||
                                    beatToFill.conditionalTitles[semConToFill.conditionSubject] !=
                                    events[i].conditionalTitles[eventSemCon.conditionSubject])
                                { continue; }

                                UnityNarreme subLiteral, obLiteral, eventSub, eventOb;
                                
                                subLiteral = beatToFill.conditionalNarremes[semConToFill.conditionSubject].GetLiteralForm<UnityNarreme>();
                                eventSub = events[i].conditionalNarremes[eventSemCon.conditionSubject];

                                if (semConToFill.conditionObject < 0)
                                {
                                    if(eventSemCon.conditionObject < 0)
                                    {
                                        if(subLiteral == eventSub) { foundCondition = true; break; }
                                    }
                                    continue;
                                }
                                else if(eventSemCon.conditionObject < 0) { continue; }

                                if (beatToFill.conditionalTitles[semConToFill.conditionObject] !=
                                    events[i].conditionalTitles[eventSemCon.conditionObject])
                                { continue; }

                                obLiteral = beatToFill.conditionalNarremes[semConToFill.conditionObject].GetLiteralForm<UnityNarreme>();
                                eventOb = events[i].conditionalNarremes[eventSemCon.conditionObject];

                                if (eventSub == subLiteral && eventOb == obLiteral) 
                                { foundCondition = true; break; }
                            }
                            if (!foundCondition) { score++; }
                        }

                        for (int j = beatToFill.causalCondition.causes.Length - 1; j >= 0; j--)
                        {
                            bool foundCause = false;
                            if (beatToFill.causalCondition.causes[j] < 0) { continue; }

                            for (int k = events[i].causalCondition.causes.Length - 1; k >= 0; k--)
                            {
                                if (events[i].causalCondition.causes[k] < 0) { continue; }

                                if (beatToFill.conditionalTitles[beatToFill.causalCondition.causes[j]] !=
                                    events[i].conditionalTitles[events[i].causalCondition.causes[k]])
                                { continue; }

                                UnityNarreme causeToFill = beatToFill.conditionalNarremes[beatToFill.causalCondition.causes[j]];
                                UnityNarreme currentCause = events[i].conditionalNarremes[events[i].causalCondition.causes[k]];

                                if (causeToFill != currentCause)
                                {
                                    UnityNarreme causeLiteral = causeToFill.GetLiteralForm<UnityNarreme>();
                                    if (causeLiteral == currentCause) { foundCause = true; break; }
                                }

                            }
                            if (!foundCause) { score++; }
                        }


                        if (score.x < highScore)
                        {
                            highScore = score.x;
                            highID = i;
                        }
                    }
                    break;

                case BestFit.Thorough:
                    for (int i = events.Length - 1; i >= 0; i--)
                    {

                        bool isMasked = false;
                        for (int cO = masked.Length - 1; cO >= 0; cO--)
                        {
                            if (i != masked[cO]) { isMasked = true; break; }
                        }
                        if (isMasked) { continue; }

                        beatToFill.semantics.CompareSemanticAffinity<Int>(events[i].semantics, out score);

                        for (int j = beatToFill.conditionalNarremes.Length - 1; j >= 0; j--)
                        {
                            bool hasTitle = false, hasNarreme = false;
                            for (int k = events[i].conditionalNarremes.Length - 1; k >= 0; k--)
                            {
                                if (events[i].conditionalTitles[k] == beatToFill.conditionalTitles[j])
                                {
                                    hasTitle = true;
                                    if (beatToFill.conditionalNarremes[j] != null)
                                    {
                                        if (beatToFill.conditionalNarremes[j] == events[i].conditionalNarremes[k])
                                        {
                                            hasNarreme = true;
                                        }
                                        else
                                        {
                                            UnityNarreme beatRelation = GetNarremeFromSymbolName(beatToFill.conditionalTitles[j]);
                                            if (beatRelation != null && beatRelation == events[i].conditionalNarremes[j])
                                            {
                                                hasNarreme = true;
                                            }
                                        }
                                    }
                                    else if (events[i].conditionalNarremes[k] == null)
                                    {
                                        hasNarreme = true;
                                    }
                                    break;
                                }
                            }
                            if (!hasTitle) { score += 2; }
                            else if (!hasNarreme) { score++; }
                        }

                        for (int j = beatToFill.narremeConditions.Length - 1; j >= 0; j--)
                        {
                            bool foundCondition = false;

                            NarremeCondition narConToFill = beatToFill.narremeConditions[j];
                            if(narConToFill.conditionSubject < 0) { continue; }

                            for (int k = events[i].narremeConditions.Length - 1; k >= 0; k--)
                            {
                                NarremeCondition eventNarCon = events[i].narremeConditions[k];
                                if(eventNarCon.conditionSubject < 0) { continue; }

                                if (narConToFill.condition != eventNarCon.condition ||
                                    beatToFill.conditionalTitles[narConToFill.conditionSubject] !=
                                    events[i].conditionalTitles[eventNarCon.conditionSubject])
                                { continue; }


                                UnityNarreme subToFill, obToFill, subLiteral, obLiteral, eventSub, eventOb;
                                subToFill = beatToFill.conditionalNarremes[narConToFill.conditionSubject];
                                subLiteral = subToFill.GetLiteralForm<UnityNarreme>();
                                eventSub = events[i].conditionalNarremes[eventNarCon.conditionSubject];

                                if (narConToFill.conditionObject < 0)
                                {
                                    if(eventNarCon.conditionObject < 0)
                                    {
                                        if (eventSub == subToFill || eventSub == subLiteral)
                                        {
                                            foundCondition = true; 
                                            break;
                                        }
                                    }
                                    continue;
                                } 
                                else if(eventNarCon.conditionObject < 0) { continue; }

                                if (beatToFill.conditionalTitles[narConToFill.conditionObject] !=
                                    events[i].conditionalTitles[eventNarCon.conditionObject])
                                { continue; }

                                obToFill = beatToFill.conditionalNarremes[narConToFill.conditionObject];
                                obLiteral = obToFill.GetLiteralForm<UnityNarreme>();
                                eventOb = events[i].conditionalNarremes[eventNarCon.conditionObject];

                                if ((eventSub == subToFill || eventSub == subLiteral) &&
                                    (eventOb == obToFill || eventOb == obLiteral)) { foundCondition = true; break; }
                            }
                            if (!foundCondition) { score++; }
                        }

                        for (int j = beatToFill.semanticConditions.Length - 1; j >= 0; j--)
                        {
                            bool foundCondition = false;

                            SemanticAffinityCondition semConToFill = beatToFill.semanticConditions[j];

                            if(semConToFill.conditionSubject < 0) { continue; }

                            for (int k = events[i].semanticConditions.Length - 1; k >= 0; k--)
                            {
                                SemanticAffinityCondition eventSemCon = events[i].semanticConditions[k];

                                if(eventSemCon.conditionSubject < 0) { continue; }

                                if (semConToFill.subjectSememe != eventSemCon.subjectSememe ||
                                    semConToFill.objectSememe != eventSemCon.objectSememe ||
                                    beatToFill.conditionalTitles[semConToFill.conditionSubject] !=
                                    events[i].conditionalTitles[eventSemCon.conditionSubject])
                                { continue; }


                                UnityNarreme subToFill, obToFill, subLiteral, obLiteral, eventSub, eventOb;
                                subToFill = beatToFill.conditionalNarremes[semConToFill.conditionSubject];
                                subLiteral = subToFill.GetLiteralForm<UnityNarreme>();
                                eventSub = events[i].conditionalNarremes[eventSemCon.conditionSubject];

                                if (semConToFill.conditionObject < 0)
                                {
                                    if (eventSub == subToFill || eventSub == subLiteral) { foundCondition = true; break; }
                                }
                                else if(eventSemCon.conditionObject < 0) { continue; }

                                if (beatToFill.conditionalTitles[semConToFill.conditionObject] !=
                                    events[i].conditionalTitles[eventSemCon.conditionObject])
                                { continue; }

                                obToFill = beatToFill.conditionalNarremes[semConToFill.conditionObject];
                                obLiteral = obToFill.GetLiteralForm<UnityNarreme>();
                                eventOb = events[i].conditionalNarremes[eventSemCon.conditionObject];

                                if ((eventSub == subToFill || eventSub == subLiteral) &&
                                    (eventOb == obToFill || eventOb == obLiteral)) { foundCondition = true; break; }
                            }
                            if (!foundCondition) { score++; }
                        }

                        for (int j = beatToFill.causalCondition.causes.Length - 1; j >= 0; j--)
                        {
                            bool foundCause = false;
                            if (beatToFill.causalCondition.causes[j] < 0) { continue; }

                            for (int k = events[i].causalCondition.causes.Length - 1; k >= 0; k--)
                            {
                                if (events[i].causalCondition.causes[k] < 0) { continue; }
                                if (beatToFill.conditionalTitles[beatToFill.causalCondition.causes[j]] !=
                                    events[i].conditionalTitles[events[i].causalCondition.causes[k]])
                                { continue; }

                                UnityNarreme causeToFill = beatToFill.conditionalNarremes[beatToFill.causalCondition.causes[j]];

                                UnityNarreme currentCause = events[i].conditionalNarremes[events[i].causalCondition.causes[k]];
                                if (causeToFill != currentCause)
                                {
                                    UnityNarreme causeLiteral = causeToFill.GetLiteralForm<UnityNarreme>();
                                    if (causeLiteral == currentCause) { foundCause = true; break; }
                                }

                            }
                            if (!foundCause) { score++; }
                        }

                        if (score.x < highScore) { highScore = score.x; highID = i; }
                    }
                    break;
            }

            return highID;
        }
        public virtual bool SetEventToBeat(int eventID, int beatID, ReplaceType replacement = ReplaceType.None) 
        {
            UnityIncident originalBeat = null;
            if (eventToBeatAssignments[beatID] >= 0)
            {
                originalBeat = events[eventToBeatAssignments[beatID]];
            }

            switch (replacement)
            {
                case ReplaceType.Symbolic:

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].title == beats[beatID].name ||
                                characters[i].relationships[j].subject == beats[beatID])
                            {
                                characters[i].relationships[j].subject = events[eventID];
                            }
                        }
                    }


                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalTitles[j] == beats[beatID].name ||
                                events[i].conditionalNarremes[j] == beats[beatID])
                            {
                                events[i].conditionalNarremes[j] = events[eventID];
                            }
                        }
                    }

                    for (int i = plots.Length - 1; i >= 0; i--)
                    {
                        for (int j = plots[i].plotIncidents.Length - 1; j >= 0; j--)
                        {
                            if (plots[i].plotIncidents[j].title == beats[beatID].name ||
                                plots[i].plotIncidents[j].plotIncident == beats[beatID])
                            {
                                plots[i].plotIncidents[j].plotIncident = events[eventID];
                            }
                        }
                    }
                    break;

                case ReplaceType.Literal:
                    if (originalBeat == null) { break; }
                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if (characters[i].relationships[j].subject == originalBeat)
                            {
                                characters[i].relationships[j].subject = events[eventID];
                            }
                        }
                    }

                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if (events[i].conditionalNarremes[j] == originalBeat)
                            {
                                events[i].conditionalNarremes[j] = events[eventID];
                            }
                        }
                    }

                    for (int i = plots.Length - 1; i >= 0; i--)
                    {
                        for (int j = plots[i].plotIncidents.Length - 1; j >= 0; j--)
                        {
                            if (plots[i].plotIncidents[j].plotIncident == originalBeat)
                            {
                                plots[i].plotIncidents[j].plotIncident = events[eventID];
                            }
                        }
                    }
                    break;

                case ReplaceType.Thorough:

                    for (int i = characters.Length - 1; i >= 0; i--)
                    {
                        for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                        {
                            if ((originalBeat != null && characters[i].relationships[j].subject == originalBeat) ||
                                characters[i].relationships[j].subject == beats[beatID] ||
                                characters[i].relationships[j].title == beats[beatID].name)
                            {
                                characters[i].relationships[j].subject = events[eventID];
                            }
                        }
                    }


                    for (int i = events.Length - 1; i >= 0; i--)
                    {
                        for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                        {
                            if ((originalBeat != null&& events[i].conditionalNarremes[j] == originalBeat) ||
                                events[i].conditionalNarremes[j] == beats[beatID] ||
                                events[i].conditionalTitles[j] == beats[beatID].name)
                            {
                                events[i].conditionalNarremes[j] = events[eventID];
                            }
                        }
                    }

                    for (int i = plots.Length - 1; i >= 0; i--)
                    {
                        for (int j = plots[i].plotIncidents.Length - 1; j >= 0; j--)
                        {
                            if ((originalBeat != null && plots[i].plotIncidents[j].plotIncident == originalBeat) ||
                                plots[i].plotIncidents[j].plotIncident == beats[beatID] ||
                                plots[i].plotIncidents[j].title == beats[beatID].name)
                            {
                                plots[i].plotIncidents[j].plotIncident = events[eventID];
                            }
                        }
                    }
                    break;
            }


            eventToBeatAssignments[beatID] = eventID;
            OnSetEventToBeat?.Invoke(eventID, beatID);
            return true;
            
        }
        public virtual void RemoveEventFromBeat(int beatID) 
        {
            if (eventToBeatAssignments[beatID] < 0) { return; }

            int eventID = eventToBeatAssignments[beatID];

            UnityIncident currentEventBeat = events[eventToBeatAssignments[beatID]];

            for (int i = characters.Length - 1; i >= 0; i--)
            {
                for (int j = characters[i].relationships.Length - 1; j >= 0; j--)
                {
                    if ((characters[i].relationships[j].subject == currentEventBeat) &&
                        characters[i].relationships[j].title == beats[beatID].name)
                    {
                        characters[i].relationships[j].subject = null;
                    }
                }
            }

            for (int i = events.Length - 1; i >= 0; i--)
            {
                for (int j = events[i].conditionalTitles.Length - 1; j >= 0; j--)
                {
                    if ((events[i].conditionalNarremes[j] == currentEventBeat) &&
                        events[i].conditionalTitles[j] == beats[beatID].name)
                    {
                        events[i].conditionalNarremes[j] = null;
                    }
                }
            }
            eventToBeatAssignments[beatID] = -1;
            OnRemoveEventFromBeat?.Invoke(eventID, beatID);
        }

        public virtual int GetBeatEvent(int beatID) { return eventToBeatAssignments[beatID]; }
        public virtual int[] GetEventBeats(int eventID)
        {
            List<int> beatIDs = new List<int>();
            for (int i = 0; i < beats.Length; i++)
            {
                if (eventToBeatAssignments[i] == eventID)
                {
                    beatIDs.Add(i);
                }
            }
            return beatIDs.ToArray();
        }

        #endregion

        #endregion

    }

    [Serializable]
    public class NarrativeText
    {
        static readonly string tagPattern = @"\[(\w+,|\w+)+\/(\w)\]";

        public string title;

        [HideInInspector]
        public string rawText;

        public NarrativeLineNotes[] lineNotes;

        [SerializeField]
        bool isParsed;

        public bool IsParsed { get { return isParsed; } }

        public string ParsedText
        {
            get
            {
                if (!isParsed) { return string.Empty; }

                System.Text.StringBuilder parsedText = new System.Text.StringBuilder();

                string nl = Environment.NewLine;

                for (int i = 0; i < lineNotes.Length - 1; i++)
                {
                    parsedText.Append(lineNotes[i].parsedLine);
                    parsedText.Append(nl);
                }

                parsedText.Append(lineNotes[lineNotes.Length - 1].parsedLine);

                return parsedText.ToString();
            }
        }

        public IEnumerator ParseRawText()
        {
            string[] lines = rawText.Split(new string[] { Environment.NewLine }, StringSplitOptions.None);

            lineNotes = new NarrativeLineNotes[lines.Length];

            Regex tagFinder = new Regex(tagPattern);

            //Step 1: Go through lines and identify narrative components and tag them.
            for (int i = 0; i < lines.Length; i++)
            {
                lineNotes[i] = new NarrativeLineNotes();

                if (string.IsNullOrWhiteSpace(lines[i])) { continue; }

                List<GeneratedTag> tags = new List<GeneratedTag>();

                var tagCollection = tagFinder.Matches(rawText);

                for (int j = 0; j < tagCollection.Count; j++)
                {
                    GeneratedTag tag = new GeneratedTag();

                    tag.SetTag(tagCollection[j].Groups[0].Value);

                    string tT = tagCollection[j].Groups[1].Value.ToLower();
                    if (tT == "c") { tag.tagType = GeneratedTagType.Character; }
                    else if (tT == "l") { tag.tagType = GeneratedTagType.Location; }
                    else if (tT == "o") { tag.tagType = GeneratedTagType.Object; }

                    tags.Add(tag);
                }

                lineNotes[i].tags = tags.ToArray();

                yield return null;
            }

            //Step 2: Begin making our marked up text.



            yield break;
        }


        public IEnumerator CreateCastOfCharacters()
        {
            yield break;
        }

        public IEnumerator CreateAtlasOfLocations()
        {
            yield break;
        }

        public IEnumerator CreateCatalogOfObjects(bool groupByLocation = true)
        {
            yield break;
        }
    }

    [Serializable]
    public class NarrativeLineNotes
    {
        [HideInInspector]
        public string parsedLine;

        public GeneratedTag[] tags;

        public int[,] errors;

        public string[] GetErrors()
        {
            return new string[0];
        }
    }

    public enum GeneratedTagType
    {
        Character,
        Object,
        Location
    }

    [Serializable]
    public class GeneratedTag
    {
        public string tag;
        public string[] aliases;
        public GeneratedTagType tagType;

        public void SetTag(string tag)
        {
            List<string> tagAndAliases = new List<string>(tag.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries));
            if (tagAndAliases.Count > 0)
            {
                tag = tagAndAliases[0];
                tagAndAliases.RemoveAt(0);
                aliases = tagAndAliases.ToArray();
            }
        }
    }

}
