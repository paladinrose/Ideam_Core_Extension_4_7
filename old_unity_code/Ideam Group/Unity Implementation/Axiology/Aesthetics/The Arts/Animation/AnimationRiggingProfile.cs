using System;
using System.Collections.Generic;

using UnityEngine;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    [CreateAssetMenu(menuName ="Ideam/Animation/Animation Rigging Profile")]
    public class AnimationRiggingProfile : ScriptableObject
    {
        public enum TermSetType
        {
            None,
            NumericalSet,
            AlphabeticSet
        }

        #region Internal Classes

        [System.Serializable]
        public class ProfileTerms
        {
            public bool caseSensitive, checkFullMatch, checkStartsWith, checkEndsWith, checkContains = true, matchAllAcceptedTerms;

            public string[] acceptedTerms = new string[0];
            public TermSetType[] acceptedTermTypes = new TermSetType[0];

            public void AddTerm(string newTerm, TermSetType termType = TermSetType.None)
            {
                List<string> newAcceptedTerms = new List<string>(acceptedTerms);
                List<TermSetType> newTermTypes = new List<TermSetType>(acceptedTermTypes);

                newAcceptedTerms.Add(newTerm);
                newTermTypes.Add(termType);

                acceptedTerms = newAcceptedTerms.ToArray();
                acceptedTermTypes = newTermTypes.ToArray();
            }

            public void RemoveTerm(int obj)
            {
                List<string> newAcceptedTerms = new List<string>(acceptedTerms);
                List<TermSetType> newTermTypes = new List<TermSetType>(acceptedTermTypes);

                newAcceptedTerms.RemoveAt(obj);
                newTermTypes.RemoveAt(obj);

                acceptedTerms = newAcceptedTerms.ToArray();
                acceptedTermTypes = newTermTypes.ToArray();
            }

            public bool CheckTerm(string term)
            {
                
                if (!caseSensitive) {term = term.ToLower(); }

                for (int i = 0; i < acceptedTerms.Length; i++)
                {
                    bool matchedTerm = false;
                    string checkTerm = acceptedTerms[i];
                    if (!caseSensitive) checkTerm = checkTerm.ToLower();

                    if (checkFullMatch && checkTerm == term) { matchedTerm = true; }
                    else if (checkStartsWith && term.StartsWith(checkTerm)) { matchedTerm = true; }
                    else if (checkEndsWith && term.EndsWith(checkTerm)) { matchedTerm = true; }
                    else if (checkContains && term.Contains(checkTerm)) { matchedTerm = true; }

                    if(matchAllAcceptedTerms && !matchedTerm) { return false; }
                    else if(!matchAllAcceptedTerms && matchedTerm) { return true; }
                }
                
                if (matchAllAcceptedTerms) { return true; }

                return false;
            }

            public bool CompareTerms(string term, string toCompare)
            {
                if (!caseSensitive)
                {
                    term = term.ToLower();
                    toCompare = toCompare.ToLower();
                }
                if (checkFullMatch && toCompare == term) { return true; }
                else if (checkStartsWith && term.StartsWith(toCompare)) { return true; }
                else if (checkEndsWith && term.EndsWith(toCompare)) { return true; }
                else if (checkContains && term.Contains(toCompare)) { return true; }

                return false;
            }

            public string TrimTerm(string term)
            {
                if (!caseSensitive) { term = term.ToLower(); }

                for (int i = 0; i < acceptedTerms.Length; i++)
                {
                    string checkTerm = acceptedTerms[i];
                    if (!caseSensitive) checkTerm = checkTerm.ToLower();

                    if (checkFullMatch && checkTerm == term) { return ""; }
                    else if (checkStartsWith && term.StartsWith(checkTerm)) { return term.Replace(checkTerm, ""); }
                    else if (checkEndsWith && term.EndsWith(checkTerm)) { return term.Replace(checkTerm, ""); }
                    else if (checkContains && term.Contains(checkTerm)) { return term.Replace(checkTerm, ""); }
                }

                return term;
            }
        }

        [System.Serializable]
        public class ConstraintProfile
        {
            [SerializeField]
            private string type;

            public string name;
            public ProfileTerms[] boneMappings = new ProfileTerms[0], 
                               controlMappings = new ProfileTerms[0];

            public string Type { get { return type; } 
                set 
                {
                    SetToType(value);
                } 
            }

            private void SetToType(string cT)
            {
                type = cT;
                int cbm = -1, ccm = -1;

                if (cT == "TwoBoneIKConstraint")
                {
                    cbm = 2; ccm = 2;
                }
                else if (cT == "BlendConstraint")
                {
                    cbm = -1; ccm = 1;
                }
                else if (cT == "MultiAimConstraint")
                {
                    cbm = 1; ccm = 1;
                }
                else if (cT == "MultiParentConstraint")
                {
                    cbm = 1; ccm = 1;
                }
                else if (cT == "MultiPositionConstraint")
                {
                    cbm = -1; ccm = 1;
                }
                else if (cT == "MultiRotationConstraint")
                {
                    cbm = -1; ccm = 1;
                }
                else if (cT == "MultiReferentialConstraint")
                {
                    cbm = 1; ccm = 1;
                }
                else if (cT == "ChainIKConstraint")
                {
                    cbm = 1; ccm = 1;
                }
                else if (cT == "TwistChainConstraint")
                {
                    cbm = 1; ccm = 2;
                }
                else if (cT == "TwistCorrection")
                {
                    cbm = 1; ccm = 1;
                }
                else if (cT == "DampedTransform")
                {
                    cbm = -1; ccm = 1;
                }
                else if (cT == "OverrideTransform")
                {
                    cbm = -1; ccm = 1;
                }

                if (boneMappings.Length < cbm)
                {
                    System.Array.Resize(ref boneMappings, cbm);
                }
                if (controlMappings.Length < ccm)
                {
                    System.Array.Resize(ref controlMappings, ccm);
                }
            }

            public void RemoveControlMapping(int toRemove)
            {
                List<ProfileTerms> newControlMappings = new List<ProfileTerms>(controlMappings);
                newControlMappings.RemoveAt(toRemove);
                controlMappings = newControlMappings.ToArray();
            }

            public void AddControlMapping(string obj)
            {
                List<ProfileTerms> newControlMappings = new List<ProfileTerms>(controlMappings);
                ProfileTerms t = new ProfileTerms() { acceptedTerms = new string[] { obj } };
                newControlMappings.Add(t);
                controlMappings = newControlMappings.ToArray();
            }

            public void RemoveBoneMapping(int obj)
            {
                List<ProfileTerms> newBoneMappings = new List<ProfileTerms>(boneMappings);
                newBoneMappings.RemoveAt(obj);
                boneMappings = newBoneMappings.ToArray();
            }

            public void AddBoneMapping(string newBoneMapping)
            {
                List<ProfileTerms> newBoneMappings = new List<ProfileTerms>(boneMappings);
                ProfileTerms t = new ProfileTerms() { acceptedTerms = new string[] { newBoneMapping } };
                newBoneMappings.Add(t);
                boneMappings = newBoneMappings.ToArray();
            }

            public void CombineWith(ConstraintProfile otherConstraint)
            {

            }
        }

        [System.Serializable]
        public class ControlProfile
        {
            public string name;
            public int parentID;
            public bool lockPosition, lockRotation;
            public Vector3 position, rotation;
        }

        [System.Serializable]
        public class RigProfileEntry
        {
            public ProfileTerms terms;

            public string generatedRigName;
            public float generatedRigWeight = 1f;
          
            public Color color;

            public ProfileTerms rootControlTerms = new ProfileTerms();
                                

            public ConstraintProfile[] generatedConstraints = new ConstraintProfile[0];
            public ControlProfile[] generatedControls = new ControlProfile[0];

            

            public ControlProfile AddControl(string controlName)
            {
                List<ControlProfile> newControls = new List<ControlProfile>(generatedControls);
                ControlProfile newControl = new ControlProfile() { name = controlName };
                newControls.Add(newControl);

                generatedControls = newControls.ToArray();
                return newControl;
            }

            public void RemoveControl(int toRemove)
            {

                List<ControlProfile> newControls = new List<ControlProfile>(generatedControls);

                newControls.RemoveAt(toRemove);

                generatedControls = newControls.ToArray();
            }

            public ConstraintProfile AddConstraint(string constraintName)
            {

                List<ConstraintProfile> newConstraints = new List<ConstraintProfile>(generatedConstraints);
                ConstraintProfile newConstraint = new ConstraintProfile() { name = constraintName };
                newConstraints.Add(newConstraint);

                generatedConstraints = newConstraints.ToArray();
                return newConstraint;
            }

            public void RemoveConstraint(int toRemove)
            {
                List<ConstraintProfile> newConstraints = new List<ConstraintProfile>(generatedConstraints);

                newConstraints.RemoveAt(toRemove);

                generatedConstraints = newConstraints.ToArray();

            }

            public void CombineWith(RigProfileEntry otherEntry)
            {
                List<ConstraintProfile> combinedConstraints = new List<ConstraintProfile>(generatedConstraints);

                for (int i = 0; i < otherEntry.generatedConstraints.Length; i++)
                {
                    ConstraintProfile otherConstraint = otherEntry.generatedConstraints[i];
                    bool addNew = true;
                    for (int j = 0; j < combinedConstraints.Count; j++)
                    {
                        if (otherConstraint.name == combinedConstraints[j].name && otherConstraint.Type == combinedConstraints[j].Type)
                        {
                            combinedConstraints[j].CombineWith(otherConstraint);
                            addNew = false;
                            break;
                        }
                    }
                    if (addNew) { combinedConstraints.Add(otherConstraint); }
                }

                List<ControlProfile> combinedControls = new List<ControlProfile>(generatedControls);

                for (int i = 0; i < otherEntry.generatedControls.Length; i++)
                {
                    ControlProfile otherControl = otherEntry.generatedControls[i];
                    bool addNew = true;
                    for (int j = 0; j < combinedControls.Count; j++)
                    {
                        if (otherControl.name == combinedControls[j].name )
                        {
                            addNew = false;
                            break;
                        }
                    }
                    if (addNew) { combinedControls.Add(otherControl); }
                }

                generatedConstraints = combinedConstraints.ToArray();
                generatedControls = combinedControls.ToArray();

                generatedRigWeight += otherEntry.generatedRigWeight;
                generatedRigWeight *= 0.5f;
            }

        }

        #endregion


        public RigProfileEntry[] profileEntries = new RigProfileEntry[0];
        
        public ProfileTerms asymmetricGroupTerms = new ProfileTerms(),
                                axialGroupTerms = new ProfileTerms(),
                                radialGroupTerms = new ProfileTerms(),
                                bilateralLeftTerms = new ProfileTerms(),
                                bilateralRightTerms = new ProfileTerms(),
                                biradialLeftTerms = new ProfileTerms(), 
                                biradialRightTerms = new ProfileTerms();

        public Mesh hintEffector, targetEffector;

        public int[] MatchingEntries(string term)
        {
            List<int> matches = new List<int>();
            for (int i = 0; i < profileEntries.Length; i++)
            {
                if (profileEntries[i].terms.CheckTerm(term)) { matches.Add(i); }
            }
            return matches.ToArray();
        }

        public RigProfileEntry NewEntry()
        {
            List<RigProfileEntry> newEntries = new List<RigProfileEntry>(profileEntries);
            RigProfileEntry newEntry = new RigProfileEntry() { generatedRigName = "New Rig Entry" };
            newEntries.Add(newEntry);
            profileEntries = newEntries.ToArray();
            return newEntry;
        }

        public void RemoveRigEntry(int obj)
        {
            List<RigProfileEntry> newEntries = new List<RigProfileEntry>(profileEntries);
            newEntries.RemoveAt(obj);
            profileEntries = newEntries.ToArray();
        }

        public void AddRigEntry(string newRigEntryName)
        {
            List<RigProfileEntry> newEntries = new List<RigProfileEntry>(profileEntries);
            RigProfileEntry newEntry = new RigProfileEntry() { generatedRigName = newRigEntryName };
            newEntries.Add(newEntry);
            profileEntries = newEntries.ToArray();
        }


        public void CombineWith(AnimationRiggingProfile otherProfile)
        {
            List<RigProfileEntry> combinedEntries = new List<RigProfileEntry>(profileEntries);
            for(int i =0; i < otherProfile.profileEntries.Length; i++)
            {
                RigProfileEntry otherEntry = otherProfile.profileEntries[i];
                bool addNew = true;
                for(int j =0; j < combinedEntries.Count; j++)
                {
                    if(combinedEntries[j].generatedRigName == otherEntry.generatedRigName)
                    {
                        combinedEntries[j].CombineWith(otherEntry);
                        addNew = false;
                        break;
                    }
                }
                if (addNew) { combinedEntries.Add(otherEntry); }
            }

            profileEntries = combinedEntries.ToArray();
        }

        public static void ApplyStandardGroupTerms(AnimationRiggingProfile profile)
        {
            profile.axialGroupTerms = new ProfileTerms()
            {
                acceptedTerms = new string[] { "hips", "pelvis", "spine", "neck", "head", "abdomen", "chest", "torso" },
                acceptedTermTypes = new TermSetType[]
               {
                    TermSetType.None, TermSetType.None, TermSetType.None,
                    TermSetType.None, TermSetType.None, TermSetType.None
               }
            };

            profile.bilateralLeftTerms = new ProfileTerms()
            {
                acceptedTerms = new string[] { "left_", "l_", "_left", "_l"},
                acceptedTermTypes = new TermSetType[]
               {
                    TermSetType.None, TermSetType.None, TermSetType.None, TermSetType.None
               }
            };

            profile.bilateralRightTerms = new ProfileTerms()
            {
                acceptedTerms = new string[] { "right_", "r_", "_right", "_r" },
                acceptedTermTypes = new TermSetType[]
               {
                    TermSetType.None, TermSetType.None, TermSetType.None, TermSetType.None
               }
            };
        }
    }
}