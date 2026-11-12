using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class AnimatorControl : BaseControl
    {
        public BlendShapeData[] blendShapeData = new BlendShapeData[0];

        [System.Serializable]
        public class BlendShapeData
        {
            public string name;
            public BlendShapesGesture blending;
            [SerializeField]
            private float currentBlendPercent;
            private float sumOfCurrentBlends, countOfCurrentBlends;
            public float CurrentBlendPercent 
            {
                get 
                {
                    return currentBlendPercent; 
                } 
                set 
                {
                    SetCurrentBlend(value);
                }
            }
            public int controlAnimatorID, blendAnimatorPropertyID;

            public void SetCurrentBlend(float value)
            {
                sumOfCurrentBlends += value;
                countOfCurrentBlends += 1f;
            }

            public void UpdateCurrentBlend()
            {
                if (countOfCurrentBlends == 0) return;
                currentBlendPercent = (sumOfCurrentBlends / countOfCurrentBlends);
                sumOfCurrentBlends = 0f; countOfCurrentBlends = 0f;

                blending.Gesture(currentBlendPercent);
            }
        }

        public void AddToBlend(int id, float blendPercent)
        {
            if(id <0 || id >= blendShapeData.Length) { return; }
            blendShapeData[id].CurrentBlendPercent += blendPercent;
            
        }

        public void BlendToValue(int id, float blendTo)
        {
            if (id < 0 || id >= blendShapeData.Length) 
            {
                if(id == -10)
                {
                    for(int i =0; i < blendShapeData.Length; i++)
                    {
                        blendShapeData[i].CurrentBlendPercent = blendTo;
                    }
                }
                return; 
            }
            blendShapeData[id].CurrentBlendPercent = blendTo;
        }

        public void UpdateBlendShapes()
        {
            for(int i =0; i < blendShapeData.Length; i++)
            {
                blendShapeData[i].UpdateCurrentBlend();
                int aid = blendShapeData[i].controlAnimatorID;
                int pid = blendShapeData[i].blendAnimatorPropertyID;
                if (aid >= 0) 
                {
                    controlAnimators[aid].SetFloat(pid, blendShapeData[i].CurrentBlendPercent);
                }
            }
        }
    }
}
