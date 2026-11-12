using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;

using UnityEditor;

using Ideam.Editor;

using Ideam.Unity.UI;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    [CustomEditor(typeof(AnimationBehaviour))]
    public class AnimationBehaviourEditor : AnimationBehaviourEditor<AnimationBehaviour, AnimationBehaviourUI>
    {
        protected override void OnEnable()
        {
            base.OnEnable();
            SetupEditorUI();
        }
    }


    public class AnimationBehaviourEditor<T, TUI> :EditorUserInterface<T, TUI> 
        where T:AnimationBehaviour
        where TUI : AnimationBehaviourUI<T>
    {
        protected override string[] ExcludedProperties()
        {
            return base.ExcludedProperties().Concat(new string[] 
            { 
                "weight" , "riggedAnimator", "animationRoots", "animationBones",  "rigging", "gestures", "groups"
            }).ToArray();
        }
        

        protected virtual void SetupEditorUI()
        {
            animationBehaviour = (T)target;
            
            animationBehaviourUI = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam/Unity Implementation/Animation/UI/AnimationBehaviour.uxml");
            animationRigUI = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam/Unity Implementation/Animation/UI/AnimationRigging.uxml");
            animationControlUI = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam/Unity Implementation/Animation/UI/AnimationRigControl.uxml");
            animationGroupUI = AssetDatabase.LoadAssetAtPath<VisualTreeAsset>("Assets/Ideam/Unity Implementation/Animation/UI/AnimationGroup.uxml");
           
        }

        public T animationBehaviour;
        public VisualTreeAsset animationBehaviourUI, animationRigUI, animationControlUI, animationGroupUI;//, animationGestureUI;
        
        protected Slider weightField;
        protected DropList riggingDrop, groupsDrop;
        //TODO: riggedAnimator, rigging, gestures, groups 
        //WHAT TO DO WITH: animationRoots, animationBones

        protected VisualElement animationBehaviourElement, selectedRigElement, selectedControlElement, selectedGroupElement;

        public override VisualElement CreateInspectorGUI()
        {
            base.CreateInspectorGUI();

            root.style.minHeight = 120;
           

            CreateAnimationBeahaviourUI();
            
            return root;
        }

        protected virtual void CreateAnimationBeahaviourUI()
        {

            animationBehaviourElement = animationBehaviourUI.CloneTree();
            root.Add(animationBehaviourElement);

            weightField = animationBehaviourElement.Q<Slider>("WeightField");
            weightField.value = animationBehaviour.Weight;
            weightField.RegisterValueChangedCallback(evt => { animationBehaviour.Weight = evt.newValue; });

            riggingDrop = animationBehaviourElement.Q<DropList>("RiggingDrop");
            string[] riggingLabels = GetRiggingLabels();
            riggingDrop.BuildList(riggingLabels);
            riggingDrop.OnSelect += SelectRigging;

            groupsDrop = animationBehaviourElement.Q<DropList>("GroupsDrop");
            string[] groupsLabels = GetGroupsLabels();
            groupsDrop.BuildList(groupsLabels);
            groupsDrop.OnSelect += SelectGroups;

        }
        
        protected virtual void SelectRigging(int id)
        {
            if(selectedRigElement != null) root.Remove(selectedRigElement);

            selectedRigElement = animationRigUI.CloneTree();
            
            root.Add(selectedRigElement);

        }

        protected virtual string[] GetRiggingLabels()
        {
            int l = animationBehaviour.rigging.Length;
            string[] rb = new string[l];
            for(int i =0; i < l; i++)
            {
                rb[i] = animationBehaviour.rigging[i].rig.gameObject.name;
            }
            return rb;
        }

        protected virtual void SelectGroups(int id)
        {
            if (selectedGroupElement != null) root.Remove(selectedGroupElement);

            selectedGroupElement = animationGroupUI.CloneTree();

            root.Add(selectedGroupElement);
        }

        protected virtual string[] GetGroupsLabels()
        {
            int l = animationBehaviour.groups.Length;
            string[] gl = new string[l];
            for (int i = 0; i < l; i++)
            {
                gl[i] = animationBehaviour.groups[i].name;
            }
            return gl;
        }

        
    }
    
    }
