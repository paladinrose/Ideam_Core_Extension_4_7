using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.Animations.Rigging;

#if UNITY_EDITOR
using UnityEditor;
#endif

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    
    public class HandAnimationRigging : AnimationBehaviour
    {
        [SerializeField]
        protected int[] handRigIDs = new int[0];

        public static AnimationRiggingProfile FreeHandRiggingProfile()
        {
            AnimationRiggingProfile handRiggingProfile = ScriptableObject.CreateInstance<AnimationRiggingProfile>();
            AnimationRiggingProfile.ApplyStandardGroupTerms(handRiggingProfile);
            AnimationRiggingProfile.RigProfileEntry freeHandRig = handRiggingProfile.NewEntry();
            freeHandRig.generatedRigName = "Free Hand Rig";

            freeHandRig.terms= new AnimationRiggingProfile.ProfileTerms();
            string[] terms = new string[]
            {
                "hand"
            };

            AnimationRiggingProfile.TermSetType[] termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None
            };

            freeHandRig.terms.acceptedTerms = terms;
            freeHandRig.terms.acceptedTermTypes = termTypes;

            freeHandRig.terms.checkContains = true;

            var handControl = freeHandRig.AddControl("Hand Root");
            handControl.parentID = -1;

            var fingerControl = freeHandRig.AddControl("*Finger Control");
            fingerControl.parentID = 0;

            var fingerHint = freeHandRig.AddControl("*Finger Hint");
            fingerHint.parentID = 0;

            var freeCon = freeHandRig.AddConstraint("Free Hand");
            freeCon.Type = "MultiParentConstraint";
            freeCon.controlMappings[0] = new AnimationRiggingProfile.ProfileTerms();

            terms = new string[]
            {
                "Hand Root"
            };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };
            freeCon.controlMappings[0].acceptedTerms = terms;
            freeCon.controlMappings[0].acceptedTermTypes = termTypes;
            freeCon.controlMappings[0].checkContains = true;

            var fingerCon = freeHandRig.AddConstraint("*Finger IK");
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
            handRiggingProfile.targetEffector = AssetDatabase.LoadAssetAtPath<Mesh>("Packages/com.unity.animation.rigging/Editor/Shapes/LocatorEffector.asset");
            handRiggingProfile.hintEffector = handRiggingProfile.targetEffector;
#endif


            return handRiggingProfile;
        }

        public static AnimationRiggingProfile IKHandRiggingProfile()
        {
            AnimationRiggingProfile handRiggingProfile = ScriptableObject.CreateInstance<AnimationRiggingProfile>();
            AnimationRiggingProfile.ApplyStandardGroupTerms(handRiggingProfile);

            AnimationRiggingProfile.RigProfileEntry ikHandRig = handRiggingProfile.NewEntry();
            ikHandRig.generatedRigName = "IK Hand Rig";
            ikHandRig.terms = new AnimationRiggingProfile.ProfileTerms();
            string[] terms = new string[]
            {
                "upper arm", "upper_arm", "upperarm", "arm_upper"
            };

            AnimationRiggingProfile.TermSetType[] termTypes = new AnimationRiggingProfile.TermSetType[]
            {
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None,
                AnimationRiggingProfile.TermSetType.None
            };

            ikHandRig.terms.acceptedTerms = terms;
            ikHandRig.terms.acceptedTermTypes = termTypes;

            ikHandRig.terms.checkContains = true;


            var handControl = ikHandRig.AddControl("Hand Control");
            handControl.parentID = -1;
            var handHint = ikHandRig.AddControl("Elbow Hint");
            handHint.parentID = -1;

           
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

            AnimationRiggingProfile.RigProfileEntry handFingerRig = handRiggingProfile.NewEntry();
            handFingerRig.generatedRigName = "Fingers Rig";

            handFingerRig.terms = new AnimationRiggingProfile.ProfileTerms();
            terms = new string[] { "hand" };
            termTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None };

            handFingerRig.terms.acceptedTerms = terms;
            handFingerRig.terms.acceptedTermTypes = termTypes;

            var linkedHandControl = handFingerRig.AddControl("<Hand Control");
            linkedHandControl.parentID = -100;

            var fingerControl = handFingerRig.AddControl("*Finger");
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
            handRiggingProfile.targetEffector = AssetDatabase.LoadAssetAtPath<Mesh>("Packages/com.unity.animation.rigging/Editor/Shapes/LocatorEffector.asset");
            handRiggingProfile.hintEffector = handRiggingProfile.targetEffector;
#endif

            return handRiggingProfile;
        }

        public static HandAnimationRigging CreateHandAnimationRigging(Animator toRig, Transform[] animationRoots, bool isIK = false)
        {
            AnimationRiggingProfile handProfile;
            if (!isIK)
            {
                handProfile = FreeHandRiggingProfile();
            }
            else
            {
                handProfile = IKHandRiggingProfile();
            }

            HandAnimationRigging handRigging = CreateGenericAnimationRig<HandAnimationRigging>(toRig, animationRoots, handProfile);
            //Link up shortcut properties, etc.


#if UNITY_EDITOR


            RigEffectorData.Style hintEffectorStyle = new RigEffectorData.Style()
            {
                shape = handProfile.hintEffector,
                size = 0.01f
            };
            
            RigEffectorData.Style targetEffectorStyle = new RigEffectorData.Style()
            {
                shape = handProfile.targetEffector,
                size = 0.01f
            };

            //Attach effectors to controls, add bone renderers to each root, etc. 
            for (int i =0; i < handRigging.rigging.Length; i++)
            {
                AnimationRigging rigging = handRigging.rigging[i];

                Rig rig = rigging.rig;
                
                hintEffectorStyle.color = targetEffectorStyle.color = rigging.rigColor;

                for(int c = 0; c < rigging.controls.Length; c++)
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

            for(int i =0; i < animationRoots.Length; i++)
            {
                BoneRenderer boneRenderer = animationRoots[i].GetComponent<BoneRenderer>();
                if (!boneRenderer)
                {
                    boneRenderer = animationRoots[i].gameObject.AddComponent<BoneRenderer>();
                }
                Transform[] bonesToRender = animationRoots[i].GetComponentsInChildren<Transform>(true);
                boneRenderer.transforms = bonesToRender;
            }
#endif
            return handRigging;
        }

        protected override AnimationRigging GenerateRigging(Animator toRig, Transform animatedRoot, Transform animatedBone, Transform rootControlObject, AnimationRiggingProfile rigProfile, int entryID, List<AnimationRigging> generatedEntries)
        {
            if(rigProfile.name.Contains("Hand Rig"))
            {
                System.Array.Resize(ref handRigIDs, handRigIDs.Length + 1);
                handRigIDs[handRigIDs.Length - 1] = generatedEntries.Count;
            }
            return base.GenerateRigging(toRig, animatedRoot, animatedBone, rootControlObject, rigProfile, entryID, generatedEntries);
        }

        public AnimationControl HandControl(int id) 
        {
            return rigging[handRigIDs[id]].controls[0];
        }
    }

}
