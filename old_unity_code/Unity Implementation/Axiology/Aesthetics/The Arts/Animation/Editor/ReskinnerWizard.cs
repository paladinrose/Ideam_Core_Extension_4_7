using System.Collections;
using System.Collections.Generic;

using UnityEditor;

using UnityEngine;
using UnityEngine.Animations.Rigging;

using Ideam.Unity.Science.Natural.Physics;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    class ReSkinnerWizard : ScriptableWizard
    {
        // public Animator animator;
        public PhysicalAnimator physicalAnimator;

        public SkinnedMeshRenderer[] toReskin;
        


        [MenuItem("Animation Rigging/Ideam/ReSkinner", false, 2000)]
        static void CreateWizard()
        {
            ScriptableWizard.DisplayWizard<ReSkinnerWizard>("ReSkinner");

        }

        private void OnWizardUpdate()
        {
            
        }

        private void OnWizardCreate()
        {
            List<PhysicsSkinner> newSkinners = new List<PhysicsSkinner>(physicalAnimator.physicsSkinners);
            int length = Mathf.Min(toReskin.Length, physicalAnimator.physicsSkinners.Length);
            for(int i =0; i < length; i++)
            {
                PhysicsSkinner oldSkinner = physicalAnimator.physicsSkinners[i];
                PhysicsSkinner newSkinner = new PhysicsSkinner() 
                {
                    skinRenderer = toReskin[i], animationRoot = oldSkinner.animationRoot,
                    physicsRoot = oldSkinner.physicsRoot, animationBones = oldSkinner.animationBones,
                    physicsBones = oldSkinner.physicsBones
                };
                newSkinner.skinRenderer.transform.SetParent(oldSkinner.skinRenderer.transform.parent, true);
                if (physicalAnimator.PhysicsEnabled)
                {
                    newSkinner.skinRenderer.rootBone = newSkinner.physicsRoot;
                    newSkinner.skinRenderer.bones = newSkinner.physicsBones;
                }
                else
                {
                    newSkinner.skinRenderer.rootBone = newSkinner.animationRoot;
                    newSkinner.skinRenderer.bones = newSkinner.animationBones;
                }
                newSkinners.Add(newSkinner);
            }
            physicalAnimator.physicsSkinners = newSkinners.ToArray();
        }

    }
}
