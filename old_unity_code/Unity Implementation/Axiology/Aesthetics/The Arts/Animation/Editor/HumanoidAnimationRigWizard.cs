using System.Collections;
using System.Collections.Generic;

using UnityEditor;
using UnityEngine;
using UnityEngine.Animations.Rigging;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    class HumanoidAnimationRigWizard : ScriptableWizard
    {
        public Animator humanoidAnimator;


        [MenuItem("Animation Rigging/Ideam/Humanoid Animation Rig", false, 2000)]
        static void CreateWizard()
        {
            ScriptableWizard.DisplayWizard<HumanoidAnimationRigWizard>("Create Humanoid Animation Rig");

        }

        private void OnWizardUpdate()
        {
            if (!humanoidAnimator) { errorString = "Plesae set a humanoid-avatar animator to continue."; }
            else { errorString = ""; }
        }

        private void OnWizardCreate()
        {
            HumanoidAnimationRigging.CreateRiggingFromAnimator(humanoidAnimator, new Transform[0]);
        }

    }
}
