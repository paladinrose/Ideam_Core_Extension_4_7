using System.Collections;
using System.Collections.Generic;

using UnityEditor;

using UnityEngine;
using UnityEngine.Animations.Rigging;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    class HandAnimationRigWizard : ScriptableWizard
    {
        public Animator animator;
        public Transform[] handRoots;
        public bool makeHandIK;

        [MenuItem("Animation Rigging/Ideam/Hand Animation Rig", false, 2000)]
        static void CreateWizard()
        {
            ScriptableWizard.DisplayWizard<HandAnimationRigWizard>("Create Hand Animation Rig");

        }

        private void OnWizardUpdate()
        {
            if (!animator)
            {
                errorString = "Please set a root level animator and at least one hand root bone (multiple hands are supported).";
            }
            else if(handRoots.Length ==0)
            {
                errorString = "Please set a root for each hand to rig.";
            }
            else
            {
                errorString = "";
            }
        }

        private void OnWizardCreate()
        {
            GameObject animObj = animator.gameObject;

            HandAnimationRigging handRigging  = HandAnimationRigging.CreateHandAnimationRigging(animator, handRoots, makeHandIK);
            
            
            EditorUtility.SetDirty(animObj);
        }

    }
}