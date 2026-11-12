using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using UnityEngine.AddressableAssets;
using UnityEngine.ResourceManagement.AsyncOperations;

using Ideam.Unity.UI;
using UnityEngine.UIElements;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class BaseControlUI<TControl> : UserInterface<TControl>
         where TControl : BaseControl
    {
        public VisualTreeAsset baseControlUXML;
        public VisualElement baseControlElement;
        
        protected AsyncOperationHandle<VisualTreeAsset> baseControlUXMLHandle;
        protected bool uxmlIsLoaded, baseControlUXMLIsLoaded;
        protected override void LoadDefaultUXML()
        {
            waitingForUXML = true;
            Type controlType = GetType();
            string uxmlPath = controlType.FullName;
            uxmlPath = uxmlPath.Replace(".", "/");
            uxmlHandle = Addressables.LoadAssetAsync<VisualTreeAsset>(uxmlPath);
            uxmlHandle.Completed += ControlUXMLComplete;

            Type baseType = typeof(BaseControl);

            string baseControlUXMLPath = baseType.FullName;
            baseControlUXMLPath = uxmlPath.Replace(".", "/");
            baseControlUXMLHandle = Addressables.LoadAssetAsync<VisualTreeAsset>(baseControlUXMLPath);
            baseControlUXMLHandle.Completed += LoadingBaseControlUIComplete;
        }

        private void ControlUXMLComplete(AsyncOperationHandle<VisualTreeAsset> obj)
        {
            if (obj.Status == AsyncOperationStatus.Succeeded)
            {
                
                uxmlIsLoaded = true;
                if (baseControlUXMLIsLoaded) { UXMLLoadingComplete(uxmlHandle); }
            }
            else { Debug.Log("Failed to load UXML."); }
        }

        private void LoadingBaseControlUIComplete(AsyncOperationHandle<VisualTreeAsset> obj)
        {
            if (obj.Status == AsyncOperationStatus.Succeeded)
            {
                baseControlUXMLIsLoaded = true;
                baseControlUXML = obj.Result;
                if (uxmlIsLoaded) { UXMLLoadingComplete(uxmlHandle); }
            }else
            {
                Debug.Log("Failed to load Base Control UXML.");
            }
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

            baseControlElement = baseControlUXML.CloneTree();
            root.Insert(0, baseControlElement);
            return true;
        }
    }
}