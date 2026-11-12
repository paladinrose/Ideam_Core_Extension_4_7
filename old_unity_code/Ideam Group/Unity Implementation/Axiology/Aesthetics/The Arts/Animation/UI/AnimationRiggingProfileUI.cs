using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;

using Ideam.Unity.UI;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    public class AnimationRiggingProfileUI : UserInterface<AnimationRiggingProfile>
    {
        public VisualTreeAsset entryAsset;
        protected DropList rigEntriesDrop;
        protected VisualElement selectedRigEntry;
        protected int selectedEntryID, selectedTermID, selectedConstraintID, selectedControlID, selectedBoneMapping, selectedControlMapping;

        protected VisualElement selectedConstraint, selectedControl;
        protected Toggle caseSensitive, checkFull, checkStart, checkEnd, checkContains;
        protected TextField rigNameField, boneMappingField;
        protected FloatField rigWeightField;
        protected DropList acceptedTerms, constraintsDrop, controlsDrop, boneMappings, controlMappings, controlMappingField;

        protected string[] labeledBoneMappings, labeledControlMappings, constraintNames, controlNames;
        public override void SubscribeToObjectEvents()
        {

        }

        public override void UnsubscribeFromObjectEvents()
        {

        }

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) return false;


            rigEntriesDrop = new DropList("Rig Entries");
            rigEntriesDrop.AllowMultiSelect = false;
            rigEntriesDrop.ShowSearch = true;

            string[] entryNames = new string[sourceObject.profileEntries.Length];
            for (int i = 0; i < entryNames.Length; i++)
            {
                entryNames[i] = sourceObject.profileEntries[i].generatedRigName;
            }

            rigEntriesDrop.BuildList(entryNames);
            rigEntriesDrop.OnSelect += SelectRigEntry;
            rigEntriesDrop.OnAdd += AddRigEntry;
            rigEntriesDrop.OnRemove += sourceObject.RemoveRigEntry;
            root.Add(rigEntriesDrop);

            return true;
        }



        #region Animation Rigging Profile Entry

        private void AddRigEntry(int obj)
        {
            sourceObject.AddRigEntry(rigEntriesDrop.listElements[obj].Label);
        }

        private void SelectRigEntry(int entryID)
        {
            if (selectedRigEntry != null) root.Remove(selectedRigEntry);

            selectedEntryID = entryID;
            if (selectedEntryID < 0) { return; }

            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];

            selectedRigEntry = entryAsset.CloneTree();
            selectedRigEntry.AddToClassList("background");

            root.Add(selectedRigEntry);

            caseSensitive = selectedRigEntry.Q<Toggle>("CaseSensitive");
            caseSensitive.value = entry.terms.caseSensitive;
            caseSensitive.RegisterValueChangedCallback(ChangeCaseSensitive);

            checkFull = selectedRigEntry.Q<Toggle>("CheckFullTerm");
            checkFull.value = entry.terms.checkFullMatch;
            checkFull.RegisterValueChangedCallback(ChangeCheckFull);

            checkStart = selectedRigEntry.Q<Toggle>("CheckStartsWith");
            checkStart.value = entry.terms.checkStartsWith;
            checkStart.RegisterValueChangedCallback(ChangeCheckStart);

            checkEnd = selectedRigEntry.Q<Toggle>("CheckEndsWith");
            checkEnd.value = entry.terms.checkEndsWith;
            checkEnd.RegisterValueChangedCallback(ChangeCheckEnd);

            checkContains = selectedRigEntry.Q<Toggle>("CheckContains");
            checkContains.value = entry.terms.checkContains;
            checkContains.RegisterValueChangedCallback(ChangeCheckContains);



            acceptedTerms = selectedRigEntry.Q<DropList>("AcceptedTerms");
            acceptedTerms.BuildList(entry.terms.acceptedTerms);
            acceptedTerms.OnAdd += AddTerm;
            acceptedTerms.OnSelect += SelectTerm;
            acceptedTerms.OnRemove += RemoveTerm;

            rigNameField = selectedRigEntry.Q<TextField>("RigNameField");
            rigNameField.value = entry.generatedRigName;
            rigNameField.RegisterValueChangedCallback(ChangeRigName);

            rigWeightField = selectedRigEntry.Q<FloatField>("RigWeightField");
            rigWeightField.value = entry.generatedRigWeight;
            rigWeightField.RegisterValueChangedCallback(ChangeRigWeight);

            constraintsDrop = selectedRigEntry.Q<DropList>("ConstraintsDrop");

            int l = entry.generatedConstraints.Length;
            constraintNames = new string[l];
            for (int i = 0; i < l; i++)
            {
                constraintNames[i] = entry.generatedConstraints[i].name;
            }
            constraintsDrop.BuildList(constraintNames);

            constraintsDrop.OnAdd += AddConstraint;
            constraintsDrop.OnSelect += SelectConstraint;
            constraintsDrop.OnRemove += RemoveConstraint;
            selectedConstraint = selectedRigEntry.Q<VisualElement>("SelectedConstraint");
            selectedConstraint.AddToClassList("background");

            controlsDrop = selectedRigEntry.Q<DropList>("ControlsDrop");

            l = entry.generatedControls.Length;
            controlNames = new string[l];
            for (int i = 0; i < l; i++)
            {
                controlNames[i] = entry.generatedControls[i].name;
            }
            controlsDrop.BuildList(controlNames);

            controlsDrop.OnAdd += AddControl;
            controlsDrop.OnSelect += SelectControl;
            controlsDrop.OnRemove += RemoveControl;

            selectedControl = selectedRigEntry.Q<VisualElement>("SelectedControl");
            selectedControl.AddToClassList("background");
        }


        private void ChangeRigWeight(ChangeEvent<float> evt)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedRigWeight = rigWeightField.value;
        }

        private void ChangeRigName(ChangeEvent<string> evt)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedRigName = evt.newValue;
        }

        private void SelectTerm(int obj)
        {
            selectedTermID = obj;
            //Qua?
        }

        private void AddTerm(int obj)
        {
            string newTerm = acceptedTerms.listElements[obj].Label;
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.terms.AddTerm(newTerm);
        }

        private void RemoveTerm(int obj)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.terms.RemoveTerm(obj);
        }

        private void ChangeCaseSensitive(ChangeEvent<bool> evt)
        {
            sourceObject.profileEntries[selectedEntryID].terms.caseSensitive = evt.newValue;

            if (evt.newValue)
            {

            }
            else
            {

            }
        }


        private void ChangeCheckFull(ChangeEvent<bool> evt)
        {
            sourceObject.profileEntries[selectedEntryID].terms.checkFullMatch = evt.newValue;
        }

        private void ChangeCheckStart(ChangeEvent<bool> evt)
        {
            sourceObject.profileEntries[selectedEntryID].terms.checkStartsWith = evt.newValue;
        }

        private void ChangeCheckEnd(ChangeEvent<bool> evt)
        {
            sourceObject.profileEntries[selectedEntryID].terms.checkEndsWith = evt.newValue;
        }

        private void ChangeCheckContains(ChangeEvent<bool> evt)
        {
            sourceObject.profileEntries[selectedEntryID].terms.checkContains = evt.newValue;
        }

        #endregion

        #region Controls

        private void SelectControl(int obj)
        {

            selectedControl.Clear();

            selectedControlID = obj;

            if (selectedControlID < 0) { return; }

            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            AnimationRiggingProfile.ControlProfile cProfile = entry.generatedControls[obj];

            TextField controlNameField = new TextField("Control Name");
            controlNameField.value = cProfile.name;
            controlNameField.RegisterValueChangedCallback(ChangeControlName);
            selectedControl.Add(controlNameField);

            DropList controlParentDrop = new DropList("Parent Control");

            controlParentDrop.BuildList(controlNames);
            controlParentDrop.SetHiddenEntries(new int[] { selectedControlID });

            controlParentDrop.AllowMultiSelect = false;
            controlParentDrop.NoneLabel = "(Control Root)";
            controlParentDrop.ShowNoneOption = true;
            controlParentDrop.ShowSearch = true;
            controlParentDrop.ShowSelection = true;
            controlParentDrop.OnSelect += SelectControlParent;
            selectedControl.Add(controlParentDrop);

            Toggle lockPosition = new Toggle("Lock Position");
            lockPosition.value = cProfile.lockPosition;
            lockPosition.RegisterValueChangedCallback(LockControlPosition);
            selectedControl.Add(lockPosition);

            Vector3Field positionField = new Vector3Field("Position");
            positionField.value = cProfile.position;
            positionField.RegisterValueChangedCallback(ChangeControlPosition);
            selectedControl.Add(positionField);

            Toggle lockRotation = new Toggle("Lock Rotation");
            lockRotation.value = cProfile.lockRotation;
            lockRotation.RegisterValueChangedCallback(LockControlRotation);
            selectedControl.Add(lockRotation);

            Vector3Field rotationField = new Vector3Field("Rotation");
            rotationField.value = cProfile.rotation;
            rotationField.RegisterValueChangedCallback(ChangeControlRotation);
            selectedControl.Add(rotationField);
        }

        private void ChangeControlName(ChangeEvent<string> evt)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedControls[selectedControlID].name = evt.newValue;
            controlNames[selectedControlID] = evt.newValue;

        }

        private void SelectControlParent(int obj)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedControls[selectedControlID].parentID = obj;
        }

        private void ChangeControlPosition(ChangeEvent<Vector3> obj)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedControls[selectedControlID].position = obj.newValue;
        }

        private void ChangeControlRotation(ChangeEvent<Vector3> obj)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedControls[selectedControlID].rotation = obj.newValue;
        }

        private void LockControlPosition(ChangeEvent<bool> evt)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedControls[selectedControlID].lockPosition = evt.newValue;
        }

        private void LockControlRotation(ChangeEvent<bool> evt)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedControls[selectedControlID].lockRotation = evt.newValue;
        }

        private void AddControl(int obj)
        {
            string newConName = controlsDrop.listElements[obj].Label;

            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];

            entry.AddControl(newConName);
        }

        private void RemoveControl(int toRemove)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];

            entry.RemoveControl(toRemove);
        }

        #endregion

        #region Constraints

        private void SelectConstraint(int obj)
        {
            selectedConstraintID = obj;

            selectedConstraint.Clear();

            if (selectedConstraintID < 0) { return; }

            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            AnimationRiggingProfile.ConstraintProfile cProfile = entry.generatedConstraints[selectedConstraintID];

            TextField constraintNameField = new TextField("Constraint Name");
            constraintNameField.value = cProfile.name;
            constraintNameField.RegisterValueChangedCallback(ChangeConstraintName);
            selectedConstraint.Add(constraintNameField);


            TextField constraintTypeField = new TextField("Constraint Type");
            constraintTypeField.value = cProfile.Type;
            constraintTypeField.isDelayed = true;
            constraintTypeField.RegisterValueChangedCallback(ChangeConstraintType);
            selectedConstraint.Add(constraintTypeField);

            //We forego building the next two drops, as we do a more detailed setup, further on.

            boneMappings = new DropList("Bone Mappings");
            boneMappings.AllowMultiSelect = false;
            boneMappings.ShowNoneOption = false;
            boneMappings.OnAdd += AddBoneMapping;
            boneMappings.OnRemove += RemoveBoneMapping;
            boneMappings.OnSelect += SelectBoneMapping;
            selectedConstraint.Add(boneMappings);

            controlMappings = new DropList("Control Mappings");
            controlMappings.AllowMultiSelect = false;
            controlMappings.ShowNoneOption = false;
            controlMappings.OnSelect += SelectControlMapping;
            controlMappings.OnAdd += AddControlMapping;
            controlMappings.OnRemove += RemoveControlMapping;
            selectedConstraint.Add(controlMappings);

            SetConstraintMappingLimits(cProfile.Type, entry, cProfile);
        }

        private void SelectControlMapping(int obj)
        {
            selectedControlMapping = obj;
            if (obj < labeledControlMappings.Length) { controlMappingField.Label = labeledControlMappings[obj]; }
            else
            {
                controlMappingField.Label = "Control Mapping #" + obj;
            }
        }

        private void SelectBoneMapping(int obj)
        {
            selectedBoneMapping = obj;
            if (obj < labeledBoneMappings.Length) { boneMappingField.label = labeledBoneMappings[obj]; }
            else
            {
                boneMappingField.label = "Bone Mapping #" + obj;
            }
        }

        private void RemoveControlMapping(int obj)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            AnimationRiggingProfile.ConstraintProfile cProfile = entry.generatedConstraints[selectedConstraintID];
            cProfile.RemoveControlMapping(obj);
        }

        private void AddControlMapping(int obj)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            AnimationRiggingProfile.ConstraintProfile cProfile = entry.generatedConstraints[selectedConstraintID];
            cProfile.AddControlMapping(controlMappings.listElements[obj].Label);
        }

        private void RemoveBoneMapping(int obj)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            AnimationRiggingProfile.ConstraintProfile cProfile = entry.generatedConstraints[selectedConstraintID];
            cProfile.RemoveBoneMapping(obj);
        }

        private void AddBoneMapping(int obj)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            AnimationRiggingProfile.ConstraintProfile cProfile = entry.generatedConstraints[selectedConstraintID];

            cProfile.AddBoneMapping(boneMappings.listElements[obj].Label);
        }



        private void ChangeConstraintName(ChangeEvent<string> evt)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.generatedConstraints[selectedConstraintID].name = evt.newValue;
        }

        private void ChangeConstraintType(ChangeEvent<string> evt)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            AnimationRiggingProfile.ConstraintProfile cProfile = entry.generatedConstraints[selectedConstraintID];

            string cT = evt.newValue;
            cProfile.Type = cT;

            SetConstraintMappingLimits(cT, entry, cProfile);

        }

        private void SetConstraintMappingLimits(string cT, AnimationRiggingProfile.RigProfileEntry rigEntry, AnimationRiggingProfile.ConstraintProfile cProfile)
        {

            int cbm = -1, ccm = -1;
            labeledBoneMappings = new string[0];
            labeledControlMappings = new string[0];

            if (cT == "TwoBoneIKConstraint")
            {
                cbm = 2; ccm = 2;
                labeledBoneMappings = new string[] { "Mid Bone", "Root Bone" };
                labeledControlMappings = new string[] { "Target", "Hint" };
            }
            else if (cT == "BlendConstraint")
            {
                cbm = -1; ccm = 1;
                labeledControlMappings = new string[] { "Blend Target" };
            }
            else if (cT == "MultiAimConstraint")
            {
                cbm = 1; ccm = 1;
                labeledBoneMappings = new string[] { "-Up Source" };
                labeledControlMappings = new string[] { "Aim Source", "+Up Source" };
            }
            else if (cT == "MultiParentConstraint")
            {
                cbm = -1; ccm = 1;
                labeledControlMappings = new string[] { "*Parent Source" };
            }
            else if (cT == "MultiPositionConstraint")
            {
                cbm = -1; ccm = 1;
                labeledControlMappings = new string[] { "*Position Source" };
            }
            else if (cT == "MultiRotationConstraint")
            {
                cbm = -1; ccm = 1;
                labeledControlMappings = new string[] { "*Rotation Source" };
            }
            else if (cT == "MultiReferentialConstraint")
            {
                cbm = 1; ccm = 1;
                labeledBoneMappings = new string[] { "*Reference Bone" };
                labeledControlMappings = new string[] { "*Reference Control" };
            }
            else if (cT == "ChainIKConstraint")
            {
                cbm = 1; ccm = 1;
                labeledBoneMappings = new string[] { "Root Bone" };
                labeledControlMappings = new string[] { "Target" };
            }
            else if (cT == "TwistChainConstraint")
            {
                cbm = 1; ccm = 2;
                labeledBoneMappings = new string[] { "Root Bone" };
                labeledControlMappings = new string[] { "Tip Target", "Root Target" };
            }
            else if (cT == "TwistCorrection")
            {
                cbm = 1; ccm = 1;
                labeledBoneMappings = new string[] { "*Twist Node" };
                labeledControlMappings = new string[] { "Target" };
            }
            else if (cT == "DampedTransform")
            {
                cbm = -1; ccm = 1;
                labeledControlMappings = new string[] { "Damper" };
            }
            else if (cT == "OverrideTransform")
            {
                cbm = -1; ccm = 1;
                labeledControlMappings = new string[] { "Override" };
            }


            if (cProfile.boneMappings.Length < cbm)
            {
                System.Array.Resize(ref cProfile.boneMappings, cbm);
            }
            if (cProfile.controlMappings.Length < ccm)
            {
                System.Array.Resize(ref cProfile.controlMappings, ccm);
            }

            int l = cProfile.boneMappings.Length;

            string[] finalBoneLabels = new string[l];

            string stickyLabel = string.Empty;
            List<string> useOnceLabels = new List<string>();

            for (int i = 0; i < cbm; i++)
            {

                if (labeledBoneMappings[i].StartsWith("*"))
                {
                    if (!string.IsNullOrEmpty(stickyLabel)) continue;

                    stickyLabel = labeledBoneMappings[i].TrimStart('*');
                }
                else if (labeledBoneMappings[i].StartsWith("-"))
                {
                    string useOnceLabel = labeledBoneMappings[i].TrimStart('-');

                    if (!useOnceLabels.Contains(useOnceLabel)) useOnceLabels.Add(useOnceLabel);
                }
            }

            List<string> toUseOnceLabels = new List<string>(useOnceLabels);
            for (int i = 0; i < l; i++)
            {
                if (labeledBoneMappings[i].StartsWith("-"))
                {
                    string useOnce = labeledBoneMappings[i].TrimStart('-');
                    if (toUseOnceLabels.Contains(useOnce))
                    {
                        finalBoneLabels[i] = useOnce;
                        toUseOnceLabels.Remove(useOnce);
                    }
                    else
                    {
                        finalBoneLabels[i] = cProfile.boneMappings[i].acceptedTerms[0];
                    }
                }
                else
                {
                    if (!string.IsNullOrEmpty(stickyLabel))
                    {
                        finalBoneLabels[i] = stickyLabel + " : " + cProfile.boneMappings[i];
                    }
                    else if (i < cbm)
                    {
                        if (!labeledBoneMappings[i].StartsWith("*"))
                        {
                            finalBoneLabels[i] = labeledBoneMappings[i] + " : " + cProfile.boneMappings[i].acceptedTerms[0];
                        }
                        else
                        {
                            finalBoneLabels[i] = cProfile.boneMappings[i].acceptedTerms[0];
                        }
                    }
                    else { finalBoneLabels[i] = cProfile.boneMappings[i].acceptedTerms[0]; }
                }
            }


            l = cProfile.controlMappings.Length;
            string[] finalControlLabels = new string[l];

            toUseOnceLabels.Clear();
            stickyLabel = string.Empty;

            for (int i = 0; i < ccm; i++)
            {

                if (labeledControlMappings[i].StartsWith("+"))
                {
                    string useOnce = labeledControlMappings[i].TrimStart('+');
                    if (!useOnce.Contains(useOnce)) { toUseOnceLabels.Add(useOnce); }
                }

                else if (labeledControlMappings[i].StartsWith("*") && !string.IsNullOrEmpty(stickyLabel))
                {
                    stickyLabel = labeledControlMappings[i].TrimStart('*');

                }
            }

            for (int i = 0; i < l; i++)
            {
                if (rigEntry.generatedControls.Length == 0) { finalControlLabels[i] = "(none)"; continue; }

                if (labeledControlMappings[i].StartsWith("+"))
                {
                    string useOnce = labeledControlMappings[i].TrimStart('+');
                    if (toUseOnceLabels.Contains(useOnce))
                    {
                        finalControlLabels[i] = useOnce + " : " + rigEntry.generatedControls[i].name;
                        toUseOnceLabels.Remove(useOnce);
                    }
                    else
                    {
                        finalControlLabels[i] = rigEntry.generatedControls[i].name;
                    }
                }
                else
                {
                    if (!string.IsNullOrEmpty(stickyLabel)) { finalControlLabels[i] = stickyLabel + " : " + rigEntry.generatedControls[i].name; }
                    else if (i < ccm)
                    {
                        if (!labeledControlMappings[i].StartsWith("*"))
                        {
                            finalControlLabels[i] = labeledControlMappings[i] + " : " + rigEntry.generatedControls[i].name;
                        }
                        else
                        {
                            finalControlLabels[i] = rigEntry.generatedControls[i].name;
                        }
                    }
                    else { finalControlLabels[i] = rigEntry.generatedControls[i].name; }
                }
            }

            controlMappings.BuildList(finalControlLabels);
            boneMappings.BuildList(finalBoneLabels);

        }

        private void AddConstraint(int obj)
        {
            string newConName = constraintsDrop.listElements[obj].Label;

            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];
            entry.AddConstraint(newConName);
        }

        private void RemoveConstraint(int toRemove)
        {
            AnimationRiggingProfile.RigProfileEntry entry = sourceObject.profileEntries[selectedEntryID];

            entry.RemoveConstraint(toRemove);

        }

        #endregion

    }
}