using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Pool;
using UnityEngine.UIElements;

namespace Ideam.Unity.UI.WorldSpace
{
    public class WorldSpaceUIExplorer : MonoBehaviour
    {
        
        public WorldSpaceUserInterface explorerRoot, labelContent, exploration;
        public Camera worldSpaceCamera;

        public string labelTemplateName;

        public Label nameLabel;

        public Toggle showExploration;

        public WorldSpaceStyleSheet labelStyleSheet;

        protected ObjectPool<WorldSpaceUIElement> labelPool;
        protected List<Transform> activeLabels;

        protected Transform explorerAnchor;
        protected Bounds explorerBounds;

        protected Vector3 explorerOffset = new Vector3(0.1f, 0.1f, 0.1f);

        private void Awake()
        {
            if (!explorerRoot) return;
            explorerRoot.OnRegisterUIWithDocument += SetupUIs;
            explorerRoot.OnReleaseUIFromDocument += ClearUIs;
        }

        protected virtual void SetupUIs()
        {
            for(int i =0; i < explorerRoot.elements.Count; i++)
            {
                VisualElement element = explorerRoot.elements[i].linkedElement;
                if(element is Label label)
                {
                    if(label.name == "NameField") { nameLabel = label; }
                }
                else if(element is Toggle toggle)
                {
                    if(toggle.name == "ShowExploration") { showExploration = toggle; }
                }
            }

            BuildLabelPool();

            if (showExploration != null && exploration)
            {
                showExploration.RegisterValueChangedCallback(evt => { ToggleExploration(); });
                ToggleExploration();
            }
        }

        protected virtual void ClearUIs()
        {

        }

        protected virtual void ToggleExploration()
        {
            exploration.gameObject.SetActive(showExploration.value); 
        }

        public virtual void UpdateWorldSpace()
        {
            if(explorerAnchor == null) { return; }
            
            Vector3 exPos = explorerAnchor.position;
            if(explorerBounds.size != Vector3.zero) 
            { exPos = explorerBounds.center; }
            
            Vector3 worldPos = (worldSpaceCamera.transform.position - exPos).normalized;
            
            worldPos.Scale(explorerOffset);
            worldPos += explorerAnchor.position;

            explorerRoot.transform.position = worldPos;
            
            explorerRoot.transform.rotation = Quaternion.LookRotation(explorerRoot.transform.position - worldSpaceCamera.transform.position);

            for(int i =0; i < activeLabels.Count; i++)
            {
                Transform l = activeLabels[i];
                //l.anchoredPosition3D = 
            }
        }


        protected virtual void BuildLabelPool()
        {
            activeLabels = new List<Transform>();
            labelPool = new ObjectPool<WorldSpaceUIElement>(
                CreateLabel,
                GetLabel,
                ReleaseLabel,
                DestroyLabel,
                true, 5, 5
                );

        }

        protected virtual WorldSpaceUIElement CreateLabel()
        {
            //WorldSpaceUIElement newLabel = labelContent.AddWorldSpaceElement(labelTemplateName, name + " Label");
            //return newLabel;
            return null;
        }

        protected virtual void GetLabel(WorldSpaceUIElement obj)
        {
            obj.gameObject.SetActive(true);
            activeLabels.Add(obj.transform);
        }

        protected virtual void ReleaseLabel(WorldSpaceUIElement obj)
        {
            activeLabels.Remove(obj.transform);
            obj.gameObject.SetActive(false);
        }

        protected virtual void DestroyLabel(WorldSpaceUIElement obj)
        {
            activeLabels.Remove(obj.transform);
            Destroy(obj.gameObject);
        }

        protected IEnumerator FadeLabel(WorldSpaceUIElement label, float startAlpha, float endAlpha, float t)
        {
            float c = 0;
            Color lc = label.meshRenderer.material.color;
            lc.a = startAlpha;
            label.meshRenderer.material.color = lc;

            float diff = endAlpha - startAlpha;
            float scalar = diff / t;

            while (c < t)
            {
                float p = c * scalar;
                lc.a = startAlpha + p;

                label.meshRenderer.material.color = lc;
                yield return null;
                c += Time.deltaTime;
            }

            lc.a = endAlpha;

            label.meshRenderer.material.color = lc;

            yield break;
        }
    }
}