using System.Collections;
using System.Collections.Generic;

using UnityEditor;

using UnityEngine;
using UnityEngine.Animations.Rigging;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;
namespace Ideam.Unity.Science.Natural.Physics
{
    class PhysicalAnimatorWizard : ScriptableWizard
    {
        public AnimationBehaviour animationBehaviour;


        [MenuItem("Animation Rigging/Ideam/Physical Animator", false, 2000)]
        static void CreateWizard()
        {
            ScriptableWizard.DisplayWizard<PhysicalAnimatorWizard>("Create Physical Animator");

        }

        private void OnWizardUpdate()
        {
            if (!animationBehaviour) { errorString = "Plesae set a configured AnimationBehaviour to continue."; }
            else { errorString = ""; }
        }

        private void OnWizardCreate()
        {
            var physicalAnimator = PhysicalAnimator.GeneratePhysicalAnimator<PhysicalAnimator>(animationBehaviour);
        }

    }
}
