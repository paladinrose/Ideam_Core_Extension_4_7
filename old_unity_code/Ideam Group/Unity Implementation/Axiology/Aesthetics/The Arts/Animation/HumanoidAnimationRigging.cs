using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.Animations.Rigging;

#if UNITY_EDITOR
using UnityEditor;
#endif

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation {
    public class HumanoidAnimationRigging : AnimationBehaviour
    {
       
        public static AnimationRiggingProfile HumanoidRiggingProfile()
        {
            AnimationRiggingProfile humanProfile = ScriptableObject.CreateInstance<AnimationRiggingProfile>();
            AnimationRiggingProfile.ApplyStandardGroupTerms(humanProfile);

            // ARTICULATION RIG ///////////////////////////////////////////////////////////////////////////////

            AnimationRiggingProfile.RigProfileEntry articulationRig = humanProfile.NewEntry();
            articulationRig.generatedRigName = "Articulation Rig";

            articulationRig.terms = new AnimationRiggingProfile.ProfileTerms();
            string[] terms = new string[]
            {
                "hips", "pelvis"
            };

            AnimationRiggingProfile.TermSetType[] termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            articulationRig.terms.acceptedTerms = terms;
            articulationRig.terms.acceptedTermTypes = termTypes;

            articulationRig.terms.checkContains = true;


            var hipControl = articulationRig.AddControl("Hips Control");
            hipControl.parentID = -1;

            var feetControl = articulationRig.AddControl("Feet Control");
            feetControl.parentID = -1;

            var handsControl = articulationRig.AddControl("Hands Control");
            handsControl.parentID = 0;

            var headControl = articulationRig.AddControl("Head Control");
            headControl.parentID = 0;

            var articulationSourceConstraint = articulationRig.AddConstraint("Articulation Soucre Constraint");
            articulationSourceConstraint.Type = "MultiReferentialConstraint";
            articulationSourceConstraint.boneMappings[0] = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "head", "hand", "foot"};
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                 AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None
            };
            articulationSourceConstraint.boneMappings[0].acceptedTerms = terms;
            articulationSourceConstraint.boneMappings[0].acceptedTermTypes = termTypes;

            articulationSourceConstraint.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "Head Control", "Hand Control", "Foot Control" };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                 AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None
            };
            articulationSourceConstraint.controlMappings[0].acceptedTerms = terms;
            articulationSourceConstraint.controlMappings[0].acceptedTermTypes = termTypes;

            // SPINE RIG  ////////////////////////////////////////////////////////////////////////////////////

            var spineRig = humanProfile.NewEntry();
            spineRig.generatedRigName = "Spine";
            terms = new string[] { "head" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };
            spineRig.terms = new AnimationRiggingProfile.ProfileTerms();
            spineRig.terms.acceptedTerms = terms;
            spineRig.terms.acceptedTermTypes = termTypes;

            //Here, we're just adding the SAME controls we generated for the hips and head in the articulation rig to the spine rig.
            hipControl = spineRig.AddControl("<Hips Control");
            hipControl.parentID = -100;

            headControl = spineRig.AddControl("<Head Control");
            headControl.parentID = -100;

            var spineChain = spineRig.AddConstraint("Spine Chain IK");
            spineChain.Type = "ChainIKConstraint";
            spineChain.boneMappings[0] = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "hips", "pelvis" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None };
            spineChain.boneMappings[0].acceptedTerms = terms;
            spineChain.boneMappings[0].acceptedTermTypes = termTypes;

            spineChain.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "Head Control" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None };
            spineChain.controlMappings[0].acceptedTerms = terms;
            spineChain.controlMappings[0].acceptedTermTypes = termTypes;

            var spineTwist = spineRig.AddConstraint("Spine Twist");
            spineTwist.Type = "TwistChainConstraint";
            spineTwist.boneMappings[0] = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "hips", "pelvis" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None };

            spineTwist.boneMappings[0].acceptedTerms = terms;
            spineTwist.boneMappings[0].acceptedTermTypes = termTypes;

            spineTwist.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "Head Control" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None };
            spineTwist.controlMappings[0].acceptedTerms = terms;
            spineTwist.controlMappings[0].acceptedTermTypes = termTypes;

            spineTwist.controlMappings[1] = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "Hips Control" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None };
            spineTwist.controlMappings[1].acceptedTerms = terms;
            spineTwist.controlMappings[1].acceptedTermTypes = termTypes;

            // HAND RIGS //////////////////////////////////////////////////////////////////////////////////////

            var handProfile = HandAnimationRigging.IKHandRiggingProfile();
            
            //By setting the rootControlTerms, we can choose a different root control for all the controls in this rig. Here, we're choosing the Hands Control. 
            //We'll do the same thing, below, for the leg rigs.
            handProfile.profileEntries[0].rootControlTerms = new AnimationRiggingProfile.ProfileTerms();
            
            terms = new string[] { "Hands Control" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };
            
            handProfile.profileEntries[0].rootControlTerms.checkContains = true;
            handProfile.profileEntries[0].rootControlTerms.caseSensitive = true;
            handProfile.profileEntries[0].rootControlTerms.acceptedTerms = terms;
            handProfile.profileEntries[0].rootControlTerms.acceptedTermTypes = termTypes;


            humanProfile.CombineWith(handProfile);


            // LEG RIGS ///////////////////////////////////////////////////////////////////////////////////////

            AnimationRiggingProfile.RigProfileEntry ikLegRig = humanProfile.NewEntry();
            ikLegRig.generatedRigName = "IK Foot Rig";

            ikLegRig.terms = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[]
            {
                "upper leg", "upper_leg", "upperleg", "leg_upper", "thigh"
            };

            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            ikLegRig.terms.acceptedTerms = terms;
            ikLegRig.terms.acceptedTermTypes = termTypes;

            ikLegRig.terms.checkContains = true;

            ikLegRig.rootControlTerms = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[] { "Feet Control" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };

            ikLegRig.rootControlTerms.checkContains = true;
            ikLegRig.rootControlTerms.caseSensitive = true;
            ikLegRig.rootControlTerms.acceptedTerms = terms;
            ikLegRig.rootControlTerms.acceptedTermTypes = termTypes;

            var footControl = ikLegRig.AddControl("Foot Control");
            footControl.parentID = -1;
            var footHint = ikLegRig.AddControl("Knee Hint");
            footHint.parentID = -1;

            var legIK = ikLegRig.AddConstraint("Leg IK");
            legIK.Type = "TwoBoneIKConstraint";
            legIK.boneMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                ">",
                "upper leg", "upper_leg", "upperleg", "leg_upper", "thigh",
                "lower leg", "lower_leg", "lowerleg", "leg_lower", "shin", "calf",
                "foot"
            };

            termTypes = new AnimationRiggingProfile.TermSetType[13];

            legIK.boneMappings[0].acceptedTerms = terms;
            legIK.boneMappings[0].acceptedTermTypes = termTypes;
            legIK.boneMappings[0].checkContains = true;

            legIK.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "*", "Control"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            legIK.controlMappings[0].acceptedTerms = terms;
            legIK.controlMappings[0].acceptedTermTypes = termTypes;
            legIK.controlMappings[0].checkContains = true;

            legIK.controlMappings[1] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "*", "Hint"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            legIK.controlMappings[1].acceptedTerms = terms;
            legIK.controlMappings[1].acceptedTermTypes = termTypes;
            legIK.controlMappings[1].checkContains = true;


            return humanProfile;
        }

        public static AnimationRiggingProfile NewHumanoidRiggingProfile()
        {
            AnimationRiggingProfile humanProfile = ScriptableObject.CreateInstance<AnimationRiggingProfile>();
            AnimationRiggingProfile.ApplyStandardGroupTerms(humanProfile);

           
            var spineRig = humanProfile.NewEntry();
            spineRig.generatedRigName = "Spine Rig";
           
            string[] terms = new string[]
            {
                "hips", "pelvis"
            };

            AnimationRiggingProfile.TermSetType[] termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            spineRig.terms = new AnimationRiggingProfile.ProfileTerms();
            spineRig.terms.acceptedTerms = terms;
            spineRig.terms.acceptedTermTypes = termTypes;


            var hipControl = spineRig.AddControl("Hips Control");
            hipControl.parentID = -1;

            var feetControl = spineRig.AddControl("Feet Control");
            feetControl.parentID = -1;

            var handsControl = spineRig.AddControl("Hands Control");
            handsControl.parentID = 0;

            var headControl = spineRig.AddControl("Head Control");
            headControl.parentID = 0;

            var hipParent = spineRig.AddConstraint("Hip Parent");
            hipParent.Type = "MultiParentConstraint";
            
            terms = new string[] { "Hips Control" }; termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };
            hipParent.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms() { acceptedTerms = terms, acceptedTermTypes = termTypes };

            var spineChain = spineRig.AddConstraint("Spine Chain IK");
            spineChain.Type = "ChainIKConstraint";
            terms = new string[] { "hips", "pelvis", "head" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None };
            spineChain.boneMappings[0] = new AnimationRiggingProfile.ProfileTerms() { acceptedTerms = terms, acceptedTermTypes = termTypes }; 
            terms = new string[] { "Head Control" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };
            spineChain.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms() { acceptedTerms = terms, acceptedTermTypes = termTypes };


            // HAND RIGS //////////////////////////////////////////////////////////////////////////////////////

            AnimationRiggingProfile.RigProfileEntry ikHandRig = humanProfile.NewEntry();
            ikHandRig.generatedRigName = "IK Hand Rig";
            ikHandRig.terms = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[]
            {
                "upper arm", "upper_arm", "upperarm", "arm_upper"
            };

            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            ikHandRig.terms.acceptedTerms = terms;
            ikHandRig.terms.acceptedTermTypes = termTypes;

            ikHandRig.terms.checkContains = true;

            var linkedHandsControl = ikHandRig.AddControl("<Hands Control");
            linkedHandsControl.parentID = -100;

            var handControl = ikHandRig.AddControl("Hand Control");
            handControl.parentID = 0;
            var handHint = ikHandRig.AddControl("Elbow Hint");
            handHint.parentID = 0;


            var freeCon = ikHandRig.AddConstraint("Arm IK");
            freeCon.Type = "TwoBoneIKConstraint";
            freeCon.boneMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                ">",
                "upper arm", "upper_arm", "upperarm", "arm_upper",
                "lower arm", "lower_arm", "lowerarm", "arm_lower", "forearm", "fore_arm",
                "hand"
            };

            termTypes = new AnimationRiggingProfile.TermSetType[12];

            freeCon.boneMappings[0].acceptedTerms = terms;
            freeCon.boneMappings[0].acceptedTermTypes = termTypes;
            freeCon.boneMappings[0].checkContains = true;

            freeCon.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "*", "Control"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            freeCon.controlMappings[0].acceptedTerms = terms;
            freeCon.controlMappings[0].acceptedTermTypes = termTypes;
            freeCon.controlMappings[0].checkContains = true;

            freeCon.controlMappings[1] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "*", "Hint"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            freeCon.controlMappings[1].acceptedTerms = terms;
            freeCon.controlMappings[1].acceptedTermTypes = termTypes;
            freeCon.controlMappings[1].checkContains = true;


            // FINGER RIGS ////////////////////////////////////////////////////////////////////////////////////////////////

            AnimationRiggingProfile.RigProfileEntry handFingerRig = humanProfile.NewEntry();
            handFingerRig.generatedRigName = "Fingers Rig";

            handFingerRig.terms = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "hand" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };

            handFingerRig.terms.acceptedTerms = terms;
            handFingerRig.terms.acceptedTermTypes = termTypes;

            var linkedHandControl = handFingerRig.AddControl("<Hand Control");
            linkedHandControl.parentID = -100;

            var fingerControl = handFingerRig.AddControl("*Finger Control");
            fingerControl.parentID = 0;
            var fingerHint = handFingerRig.AddControl("*Finger Hint");
            fingerHint.parentID = 0;

            var fingerCon = handFingerRig.AddConstraint("*Finger IK");
            fingerCon.Type = "TwoBoneIKConstraint";
            fingerCon.boneMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                ">", "index", "pointer", "middle", "ring", "pinky", "little", "thumb"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.NumericalSet,
                AnimationRiggingProfile.TermSetType.NumericalSet,
                AnimationRiggingProfile.TermSetType.NumericalSet,
                AnimationRiggingProfile.TermSetType.NumericalSet,
                AnimationRiggingProfile.TermSetType.NumericalSet,
                AnimationRiggingProfile.TermSetType.NumericalSet,
                AnimationRiggingProfile.TermSetType.NumericalSet,
            };

            fingerCon.boneMappings[0].acceptedTerms = terms;
            fingerCon.boneMappings[0].acceptedTermTypes = termTypes;
            fingerCon.boneMappings[0].checkContains = true;

            fingerCon.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "*", "Control"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            fingerCon.controlMappings[0].acceptedTerms = terms;
            fingerCon.controlMappings[0].acceptedTermTypes = termTypes;
            fingerCon.controlMappings[0].checkContains = true;

            fingerCon.controlMappings[1] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "*", "Hint"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            fingerCon.controlMappings[1].acceptedTerms = terms;
            fingerCon.controlMappings[1].acceptedTermTypes = termTypes;
            fingerCon.controlMappings[1].checkContains = true;

#if UNITY_EDITOR
            humanProfile.targetEffector = AssetDatabase.LoadAssetAtPath<Mesh>("Packages/com.unity.animation.rigging/Editor/Shapes/LocatorEffector.asset");
            humanProfile.hintEffector = humanProfile.targetEffector;
#endif


            // LEG RIGS ///////////////////////////////////////////////////////////////////////////////////////

            AnimationRiggingProfile.RigProfileEntry ikLegRig = humanProfile.NewEntry();
            ikLegRig.generatedRigName = "IK Foot Rig";

            ikLegRig.terms = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[]
            {
                "upper leg", "upper_leg", "upperleg", "leg_upper", "thigh"
            };

            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            ikLegRig.terms.acceptedTerms = terms;
            ikLegRig.terms.acceptedTermTypes = termTypes;

            ikLegRig.terms.checkContains = true;

            ikLegRig.rootControlTerms = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[] { "Feet Control" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };

            ikLegRig.rootControlTerms.checkContains = true;
            ikLegRig.rootControlTerms.caseSensitive = true;
            ikLegRig.rootControlTerms.acceptedTerms = terms;
            ikLegRig.rootControlTerms.acceptedTermTypes = termTypes;

            var footControl = ikLegRig.AddControl("Foot Control");
            footControl.parentID = -1;
            var footHint = ikLegRig.AddControl("Knee Hint");
            footHint.parentID = -1;

            var legIK = ikLegRig.AddConstraint("Leg IK");
            legIK.Type = "TwoBoneIKConstraint";
            legIK.boneMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                ">",
                "upper leg", "upper_leg", "upperleg", "leg_upper", "thigh",
                "lower leg", "lower_leg", "lowerleg", "leg_lower", "shin", "calf",
                "foot"
            };

            termTypes = new AnimationRiggingProfile.TermSetType[13];

            legIK.boneMappings[0].acceptedTerms = terms;
            legIK.boneMappings[0].acceptedTermTypes = termTypes;
            legIK.boneMappings[0].checkContains = true;

            legIK.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "*", "Control"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            legIK.controlMappings[0].acceptedTerms = terms;
            legIK.controlMappings[0].acceptedTermTypes = termTypes;
            legIK.controlMappings[0].checkContains = true;

            legIK.controlMappings[1] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "*", "Hint"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            legIK.controlMappings[1].acceptedTerms = terms;
            legIK.controlMappings[1].acceptedTermTypes = termTypes;
            legIK.controlMappings[1].checkContains = true;


            return humanProfile;
        }


        public static HumanoidAnimationRigging CreateRiggingFromAnimator(Animator animator, Transform[] roots)
        {
            AnimationRiggingProfile humanProfile = NewHumanoidRiggingProfile();
            if (roots.Length == 0)
            {
                Transform root = animator.GetBoneTransform(HumanBodyBones.Hips);

                SkinnedMeshRenderer skin = animator.GetComponentInChildren<SkinnedMeshRenderer>();
                if (skin)
                {
                    root = skin.rootBone;
                }
                roots = new Transform[] { root };
            }
            HumanoidAnimationRigging humanRigging = CreateGenericAnimationRig<HumanoidAnimationRigging>(animator, roots, humanProfile);

#if UNITY_EDITOR


            RigEffectorData.Style hintEffectorStyle = new RigEffectorData.Style()
            {
                shape = humanProfile.hintEffector,
                size = 0.01f
            };

            RigEffectorData.Style targetEffectorStyle = new RigEffectorData.Style()
            {
                shape = humanProfile.targetEffector,
                size = 0.01f
            };

            //Attach effectors to controls, add bone renderers to each root, etc. 
            for (int i = 0; i < humanRigging.rigging.Length; i++)
            {
                AnimationRigging rigging = humanRigging.rigging[i];

                Rig rig = rigging.rig;

                hintEffectorStyle.color = targetEffectorStyle.color = rigging.rigColor;

                for (int c = 0; c < rigging.controls.Length; c++)
                {
                    //Effectors!!
                    var con = rigging.controls[c];
                    string efName = con.transform.name;

                    if (efName.Contains("Hint"))
                    {
                        rig.AddEffector(con.transform, hintEffectorStyle);
                    }
                }
            }

            Transform hipRoot = animator.GetBoneTransform(HumanBodyBones.Hips);
            BoneRenderer boneRenderer = hipRoot.GetComponent<BoneRenderer>();
            if (!boneRenderer)
            {
                boneRenderer = hipRoot.gameObject.AddComponent<BoneRenderer>();
            }
            Transform[] bonesToRender = hipRoot.GetComponentsInChildren<Transform>(true);
            boneRenderer.transforms = bonesToRender;
            
#endif
            return humanRigging;
        }


    }

}
