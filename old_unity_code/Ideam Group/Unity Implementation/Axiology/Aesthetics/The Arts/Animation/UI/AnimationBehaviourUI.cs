using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;


using UnityEngine.AddressableAssets;
using UnityEngine.ResourceManagement.AsyncOperations;

using Ideam.Unity.UI;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    [UserInterface(typeof(AnimationBehaviour))]
    public class AnimationBehaviourUI : AnimationBehaviourUI<AnimationBehaviour> { }
    
    [UserInterface(typeof(HandAnimationRigging))]
    public class HandAnimationRiggingUI : AnimationBehaviourUI<HandAnimationRigging> { }

    [UserInterface(typeof(HumanoidAnimationRigging))]
    public class HumanoidAnimationRiggingUI : AnimationBehaviourUI<HumanoidAnimationRigging> { }


    public class AnimationBehaviourUI<T> : UserInterface<T>
        where T:AnimationBehaviour
    {
        public override event Action<UserInterface> OnDefaultUXMLLoaded;
        public T animationBehaviour;
        public VisualTreeAsset animationBehaviourUI, animationRigUI, animationControlUI, animationGroupUI;//, animationGestureUI;

        protected Slider weightField;
        protected DropList riggingDrop, groupsDrop;
        //TODO: riggedAnimator, rigging, gestures, groups 
        //WHAT TO DO WITH: animationRoots, animationBones

        protected VisualElement animationBehaviourElement, selectedRigElement, selectedControlElement, selectedGroupElement;
        protected override void LoadDefaultUXML()
        {
            if (waitingForUXML) { return; }

            waitingForUXML = true;
            loadingLabel.style.display = DisplayStyle.Flex;

            string uxmlPath = GetType().FullName;
            uxmlPath = uxmlPath.Replace(".", "/");
            uxmlHandle = Addressables.LoadAssetAsync<VisualTreeAsset>(uxmlPath);
            uxmlHandle.Completed += evt => 
            {
                if(evt.Status == AsyncOperationStatus.Succeeded)
                {
                    uxml = evt.Result;
                    if (ValidateUXMLAssets()) { UXMLLoadingComplete(uxmlHandle); }
                }
            };
        }

        protected virtual bool ValidateUXMLAssets()
        {
            if (uxml == null ||
                animationRigUI == null ||
                animationControlUI == null ||
                animationGroupUI == null) return false;

            return true;
        }

        protected override void UXMLLoadingComplete(AsyncOperationHandle<VisualTreeAsset> obj)
        {
            waitingForUXML = false;
            loadingLabel.style.display = DisplayStyle.None;
            if (isOpen)
            {
                SetupUIFromUXML();
            }
            OnDefaultUXMLLoaded?.Invoke(this);
        }

        public override void SubscribeToObjectEvents()
        {

        }

        public override void UnsubscribeFromObjectEvents()
        {

        }

        protected override bool SetupUIFromUXML()
        {
            if (!base.SetupUIFromUXML()) { return false; }


            return true;
        }
    }
}
