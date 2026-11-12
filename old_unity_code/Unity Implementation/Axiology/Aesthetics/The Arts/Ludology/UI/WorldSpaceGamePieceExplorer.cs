using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.UIElements;
using UnityEngine.Pool;


using Ideam.Unity;
using Ideam.Science.Formal.Mathematics;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.UI.WorldSpace
{
    public class WorldSpaceGamePieceExplorer : WorldSpaceUIExplorer
    {
        public UnityGamePiece gamePiece;
        public bool showPropertyChanges, showResourceChanges, showResourceUses, showResourceMaxChanges, showResourcesExhaustion;
       
        public Transform resourceContent;

        protected override void SetupUIs()
        {
            base.SetupUIs();

            if (gamePiece)
            {
                var gp = gamePiece;
                ClearGamePiece();
                SetGamePiece(gp);
            }
        }


        public virtual void SetGamePiece(UnityGamePiece gP, string nameFieldOverride = "")
        {
            if(gamePiece == gP) { return; }
            
            if (gamePiece) { ClearGamePiece(); }

            gamePiece = gP;
            explorerAnchor = gamePiece.transform;

            explorerBounds = explorerAnchor.PhysicsLocalBounds(true);

            //Here, after more sleep, I want to use the bounds to figure out the explorerOffset, which should ultimately be a
            //transformed direction from exploererAnchor - at least, I think so. I think collider bounds are measured along local axes.

            if (nameLabel != null)
            {
                if (nameFieldOverride != string.Empty)
                {
                    nameLabel.text = nameFieldOverride;
                }
                else { nameLabel.text = gamePiece.name; }
            }
            if (showPropertyChanges)
            {
                for (int i = 0; i < gamePiece.properties.Length; i++)
                {
                    UnityGamePieceProperty property = gamePiece.properties[i];
                    property.OnValueChanged += ShowPropertyChange;

                    gamePiece.properties[i] = property;
                }
            }


            for (int i = 0; i < gamePiece.resources.Length; i++)
            {
                UnityGamePieceResource resource = gamePiece.resources[i];
                if (showResourceChanges)
                {
                    resource.OnValueChanged += ShowResourceChange;
                }

                if (showResourceMaxChanges)
                {
                    resource.OnMaxValueChanged += ShowResourceMaxChange;
                }

                if (showResourceUses)
                {
                    resource.OnUse += ShowResourceUse;
                    resource.OnCannotUse += ShowCannotUseResource;
                }

                if (showResourcesExhaustion)
                {
                    resource.OnExhausted += ShowResourceExhaustion;
                }

                gamePiece.resources[i] = resource;

            }
        }

        protected virtual void ClearGamePiece()
        {
            for (int i = 0; i < gamePiece.resources.Length; i++)
            {
                UnityGamePieceResource resource = gamePiece.resources[i];
                if (showResourceChanges)
                {
                    resource.OnValueChanged -= ShowResourceChange;
                }

                if (showResourceMaxChanges)
                {
                    resource.OnMaxValueChanged -= ShowResourceMaxChange;
                }

                if (showResourceUses)
                {
                    resource.OnUse -= ShowResourceUse;
                    resource.OnCannotUse -= ShowCannotUseResource;
                }

                if (showResourcesExhaustion)
                {
                    resource.OnExhausted -= ShowResourceExhaustion;
                }

                gamePiece.resources[i] = resource;

            }
        }

        #region Exploration
        
        public virtual void OpenInfoExploration() { }

        public virtual void StartConversation()
        {

        }

        #endregion


        #region Resources

        private void ShowResourceChange(UnityGamePieceResource obj, Int oldValue)
        {
            
            Int diff = obj.CurrentValue - oldValue;

            float s = oldValue/obj.MaxValue;
            float e = obj.CurrentValue/obj.MaxValue;
            float t = 1.5f;


            WorldSpaceUIElement resourceLabel = labelPool.Get();
            if(resourceLabel.linkedElement is Label lab)
            {
                lab.text = diff.ToString();
                if(diff < 0)
                {
                    lab.AddToClassList("negative");
                }
                else
                {
                    lab.RemoveFromClassList("negative");
                }
            }
            
            resourceLabel.transform.SetParent(resourceContent, false);

            IEnumerator[] resLabelAnimations = new IEnumerator[2];
            resLabelAnimations[0] = FadeLabel(resourceLabel, 0f, 1f, 0.25f);
            resLabelAnimations[2] = FadeLabel(resourceLabel, 1f, 0f, 1.75f);
            StartCoroutine(ResourceAnimationCombiner(resLabelAnimations));
        }

        private void ShowResourceUse(UnityGamePieceResource obj)
        {
            ShowResourceChange(obj, obj.CurrentValue + 1);
        }

        private void ShowResourceExhaustion(UnityGamePieceResource obj)
        {
            throw new NotImplementedException();
        }

        private void ShowCannotUseResource(UnityGamePieceResource obj)
        {
            throw new NotImplementedException();
        }

        private void ShowResourceMaxChange(UnityGamePieceResource arg1, Int arg2)
        {
            throw new NotImplementedException();
        }



        IEnumerator ResourceAnimationCombiner(IEnumerator[] subAnimations)
        {
            for (int i = 0; i < subAnimations.Length; i++)
            {
                IEnumerator subAnimation = subAnimations[i];
                while (!subAnimation.MoveNext()) { yield return null; }
            }
        }

        #endregion


        private void ShowPropertyChange(UnityGamePieceProperty obj, Int oldValue)
        {
            throw new NotImplementedException();
        }

        
    }
}