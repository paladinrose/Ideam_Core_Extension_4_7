using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

using Ideam.Editor;
using Ideam.Unity.UI;
using UnityEngine.UIElements;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    [CustomEditor(typeof(AnimationRiggingProfile))]
    public class AnimationRiggingProfileEditor :EditorUserInterface<AnimationRiggingProfile, AnimationRiggingProfileUI>
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[] { "profileEntries"}).ToArray();
        }

        protected override void OnEnable()
        {
            profile = (AnimationRiggingProfile)target;
            entryAsset = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam/Unity Implementation/Animation/UI/RigProfileEntry.uxml");
        }

        public AnimationRiggingProfile profile;
        public VisualTreeAsset entryAsset;
        DropList rigEntriesDrop;
        VisualElement selectedRigEntry;
        int selectedEntryID, selectedTermID, selectedConstraintID, selectedControlID, selectedBoneMapping, selectedControlMapping;

        VisualElement selectedConstraint, selectedControl;
        Toggle caseSensitive, checkFull, checkStart, checkEnd, checkContains;
        TextField rigNameField, boneMappingField;
        FloatField rigWeightField;
        DropList acceptedTerms, constraintsDrop, controlsDrop, boneMappings, controlMappings, controlMappingField;

        string[] labeledBoneMappings, labeledControlMappings, constraintNames, controlNames;

        public override VisualElement CreateInspectorGUI()
        {
            base.CreateInspectorGUI();
           
            root.style.minHeight = 120;
            
            return root;
        }


        
    }
}