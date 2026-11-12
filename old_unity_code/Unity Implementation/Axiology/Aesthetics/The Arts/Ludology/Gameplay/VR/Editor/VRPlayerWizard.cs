
using UnityEditor;

using UnityEngine;
using Cinemachine;
using UnityEngine.Animations.Rigging;
//using UnityEngine.SpatialTracking;
using UnityEngine.InputSystem.XR;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Science.Natural.Physics;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{
    class VRPlayerWizard : ScriptableWizard
    {
        public UnityGame game = null;
        public Animator animator = null;
        public enum AnimationRiggingType { Generic, FreeHand, IKHand, Humanoid};

        public AnimationRiggingType riggingType;
        public AnimationRiggingProfile animationRiggingProfile = null;
        public bool createVRUI = true, createRigidbody = true, createCapsuleCollider = true, createPhysicalAnimator;

        public Transform[] animationRoots;


        [MenuItem("Tools/Ideam/Gameplay/VR/VR Player", false, 2000)]
        static void CreateWizard()
        {
            DisplayWizard<VRPlayerWizard>("Create VR Player");
        }

        private void OnWizardUpdate()
        {
            /*
            if (!animator) 
            {
                errorString = "Please set an animator for the VR Player to continue."; 
            }
            else
            {
                if(animationRiggingProfile != null && riggingType != VRRiggingType.Generic)
                {
                    errorString = "Animation Rigging Profile will only be used if Rigging Typ is Generic.";
                }
                else if(riggingType == VRRiggingType.Generic && animationRiggingProfile == null)
                {
                    errorString = "Using a Generic Rigging Type requires an Animation Rigging Profile to be set.";
                }
                else { errorString = ""; }
            }

            isValid = string.IsNullOrEmpty(errorString);

            if( isValid && riggingType == VRRiggingType.Humanoid && !animator.isHuman)
            {
                errorString = "Warning: Animator does not seem to be a humanoid animator. You may proceed to try and rig it, however.";
            }
            */
        }

        private void OnWizardCreate()
        {

            VRPlayer player = VRPlayer.NewVRPlayer(animator);
            GameObject playerObject = player.gameObject;

            Rigidbody agentBody = null;
            CapsuleCollider cap = null;

            if (createRigidbody)
            {
                agentBody = playerObject.AddComponent<Rigidbody>();
                agentBody.mass = 0.001f;
                agentBody.drag = 0f;
                agentBody.angularDrag = 0f;
                agentBody.constraints = RigidbodyConstraints.FreezeRotation;
                agentBody.isKinematic = true;
                agentBody.useGravity = false;
                agentBody.interpolation = RigidbodyInterpolation.Interpolate;
                agentBody.collisionDetectionMode = CollisionDetectionMode.ContinuousDynamic;
            }

            

            GameObject headObject = new GameObject("Head Controller"),
                       leftHandObject = new GameObject("Left Hand Controller"),
                       rightHandObject = new GameObject("Right Hand Controller");
            
            VRGhost agent = VRGhost.NewVRGhost(playerObject.transform, animator, agentBody, playerObject.transform, headObject.transform, game);
            
            VRController controller = VRController.AddVRController(playerObject, agent, headObject, leftHandObject, rightHandObject, agentBody);
            player.characterController = controller;

            VRHeadController headController = controller.headController;
            VRHandController leftHandController = controller.leftHandController,
                             rightHandController = controller.rightHandController;

            if (createCapsuleCollider)
            {
                cap = playerObject.AddComponent<CapsuleCollider>();
                cap.center = new Vector3(0, controller.StandingHeight * 0.5f, 0);
                cap.height = controller.StandingHeight;
                cap.radius = 0.15f;
            }

            GameObject ghostObject = agent.gameObject;
            ghostObject.name = "VR Body";

            player.SetGhostAgent(agent);
            controller.agent = agent;

            headController.peripheralVision = agent.mind.sight.peripheralVision;

            headController.transform.SetParent(ghostObject.transform, true);
            leftHandController.transform.SetParent(ghostObject.transform, true);
            rightHandController.transform.SetParent(ghostObject.transform, true);

            PhysicalAnimator physicalAnimator = null;
            AnimationBehaviour animationBehaviour = null;

            if (createVRUI)
            {
                VRUI vrui = VRUI.NewVRUI(headController.transform);
                controller.vrui = headController.vrui = leftHandController.vrui = rightHandController.vrui = vrui;

                headController.bothEyesUI.worldSpaceDocument = 
                headController.leftEyeUI.worldSpaceDocument =
                headController.rightEyeUI.worldSpaceDocument = vrui.worldSpaceUIDocument;

                vrui.game = game;
                vrui.vrAgent = agent;

                vrui.leftMenu.transform.SetParent(leftHandController.transform);
                vrui.rightMenu.transform.SetParent(rightHandController.transform);

                 
            }

            if (animator != null)
            {
                switch (riggingType)
                {
                    case AnimationRiggingType.Generic:
                        animationBehaviour = AnimationBehaviour.CreateGenericAnimationRig<AnimationBehaviour>(animator, animationRoots, animationRiggingProfile);
                        break;
                    case AnimationRiggingType.FreeHand:
                        var freeHandBehaviour = HandAnimationRigging.CreateHandAnimationRigging(animator, animationRoots);
                        animationBehaviour = freeHandBehaviour;
                        break;
                    case AnimationRiggingType.IKHand:
                        var ikHandBehaviour = HandAnimationRigging.CreateHandAnimationRigging(animator, animationRoots, true);
                        animationBehaviour = ikHandBehaviour;
                        break;
                    case AnimationRiggingType.Humanoid:
                        var humanoidBehaviour = HumanoidAnimationRigging.CreateRiggingFromAnimator(animator, animationRoots);
                        animationBehaviour = humanoidBehaviour;

                        //Once more, this is stuff I'd like to find a better place for, later on.
                        //var hipBone = animator.GetBoneTransform(HumanBodyBones.Hips);
                        //humanoidBehaviour.rigging[0].controls[0].transform.MoveIndependent(hipBone.position);
                        Vector3 handsPos = animator.GetBoneTransform(HumanBodyBones.LeftHand).position + animator.GetBoneTransform(HumanBodyBones.RightHand).position;
                        handsPos *= 0.5f;
                        humanoidBehaviour.rigging[0].controls[2].transform.MoveIndependent(handsPos);
                        break;
                }
            }

            if (animationBehaviour)
            {
                agent.ghostHandRig = animationBehaviour;

                var handControlSearchTerms = new AnimationRiggingProfile.ProfileTerms()
                {
                    acceptedTerms = new string[] { "Hand Control", "_l" },
                    acceptedTermTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None },
                    matchAllAcceptedTerms = true
                };

                var leftHandConts = animationBehaviour.Controls(handControlSearchTerms);
                if(leftHandConts.Length > 0)
                {
                    leftHandController.handControl =leftHandConts[0];
                    leftHandController.handControl.Lock(leftHandController.transform);
                    leftHandController.handControl.lockSettings.shadowPosition = leftHandController.handControl.lockSettings.shadowRotation = true;
                }
                
                handControlSearchTerms = new AnimationRiggingProfile.ProfileTerms()
                {
                    acceptedTerms = new string[] { "Hand Control", "_r" },
                    acceptedTermTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None, AnimationRiggingProfile.TermSetType.None },
                    matchAllAcceptedTerms = true
                };

                var rightHandConts = animationBehaviour.Controls(handControlSearchTerms);
                if (rightHandConts.Length > 0)
                {
                    rightHandController.handControl = rightHandConts[0];
                    rightHandController.handControl.Lock(rightHandController.transform);
                    rightHandController.handControl.lockSettings.shadowPosition = rightHandController.handControl.lockSettings.shadowRotation = true;
                }


                var headTerms = new AnimationRiggingProfile.ProfileTerms()
                {
                    acceptedTerms = new string[] { "head" },
                    acceptedTermTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None }
                };
                var headControls = animationBehaviour.Controls(headTerms);
                if (headControls.Length > 0)
                {
                    headController.headControl = headControls[0];
                    headController.headControl.Lock(headController.transform);
                    headController.headControl.lockSettings.shadowPosition = headController.headControl.lockSettings.shadowRotation = true;
                }

                var hipControlSearchTerms = new AnimationRiggingProfile.ProfileTerms()
                {
                    acceptedTerms = new string[] { "Hips Control" },
                    acceptedTermTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None },
                    matchAllAcceptedTerms = true
                };

                var hipControls = animationBehaviour.Controls(hipControlSearchTerms);
                if(hipControls.Length > 0)
                {
                    //controller.crouchController = hipControls[0].transform;
                    ghostObject.transform.SetParent(hipControls[0].transform, false);
                    ghostObject.transform.position = animator.transform.position;
                    ghostObject.transform.rotation = animator.transform.rotation;
                }

                if (createPhysicalAnimator)
                {

                    physicalAnimator = PhysicalAnimator.GeneratePhysicalAnimator<PhysicalAnimator>(animationBehaviour);
                    agent.ghostPhysics = physicalAnimator;
                    //controller.physicalAnimator = physicalAnimator;

                    physicalAnimator.ChangeBoneJoint(0, "Ball");
                    physicalAnimator.physicsBones[0].joint.connectedBody = agentBody;

                    if (riggingType == AnimationRiggingType.Humanoid) {

                        //1. Find the Axial group.
                        AnimationGroup axialGroup = animationBehaviour.groups[animationBehaviour.HasGroup("Axial")];

                        //2. Set Pelvis and all Spine members to use box colliders.
                        int hipGroupID = -1, 
                            lastSpineGroupID = -1;

                        Vector3 pelvisSize = Vector3.zero, shoulderSize = Vector3.zero;

                        for (int i = 0; i < axialGroup.boneIDs.Length; i++)
                        {
                            int id = axialGroup.boneIDs[i];
                            string n = axialGroup.bones[i].name.ToLower();
                            
                            
                            if(n.Contains("pelvis") || n.Contains("hips"))
                            {
                                hipGroupID = i;
                                physicalAnimator.ChangeBoneCollider(id, "Box");
                                //3. Scale the pelvis to encompass both thighs and spine points
                                BoxCollider hipCollider = (BoxCollider)physicalAnimator.physicsBones[id].collider;
                                pelvisSize = hipCollider.size;
                                
                            }
                            else if (n.Contains("neck"))
                            {
                                lastSpineGroupID = i - 1;
                                id = axialGroup.boneIDs[lastSpineGroupID];
                                Transform lastSpine = physicalAnimator.physicsBones[id].transform;
                                BoxCollider lastSpineBox = (BoxCollider)physicalAnimator.physicsBones[id].collider;

                                int shouldersID = animationBehaviour.HasGroup("upperarm");
                                AnimationGroup shoulders = animationBehaviour.groups[shouldersID];
                                
                                //4. Get the Upper Arms. Set the last Spine to encompass them, and the neck origin.
                                Bounds shoulderBounds = new Bounds(lastSpineBox.center, lastSpineBox.size);

                                shoulderBounds.Encapsulate(lastSpine.InverseTransformPoint(shoulders.bones[0].position));
                                shoulderBounds.Encapsulate(lastSpine.InverseTransformPoint(shoulders.bones[1].position));
                                

                                lastSpineBox.center = shoulderBounds.center;
                                shoulderSize = lastSpineBox.size = shoulderBounds.size;
                            }
                            else
                            {
                                physicalAnimator.ChangeBoneCollider(id, "Box");
                            }
                        }

                        //5. For each OTHER spine segment, start at a base, small size (Figure out how to get the width of the thigh capsule).
                        int widthDirection = -1;
                        float baseWidth = 0, hipWidth = 0, shoulderWidth = 0;

                        if(pelvisSize.x > pelvisSize.y)
                        {
                            if(pelvisSize.x > pelvisSize.z)
                            {
                                //x
                                widthDirection = 0;
                                hipWidth = pelvisSize.x;
                                shoulderWidth = shoulderSize.x;
                                baseWidth = pelvisSize.x * 0.5f;

                                if(pelvisSize.y < baseWidth) { pelvisSize.y = baseWidth; }
                                if (pelvisSize.z < baseWidth) { pelvisSize.z = baseWidth; }
                            }
                            else
                            {
                                //z
                                widthDirection = 2;
                                hipWidth = pelvisSize.z;
                                shoulderWidth = shoulderSize.z;
                                baseWidth = pelvisSize.z * 0.5f;

                                if (pelvisSize.y < baseWidth) { pelvisSize.y = baseWidth; }
                                if (pelvisSize.x < baseWidth) { pelvisSize.x = baseWidth; }
                            }
                        }
                        else if(pelvisSize.y > pelvisSize.z)
                        {
                            //y
                            widthDirection = 1;
                            hipWidth = pelvisSize.y;
                            shoulderWidth = shoulderSize.y;
                            baseWidth = pelvisSize.y * 0.5f;

                            if (pelvisSize.x < baseWidth) { pelvisSize.x = baseWidth; }
                            if (pelvisSize.z < baseWidth) { pelvisSize.z = baseWidth; }
                        }
                        else
                        {
                            //z
                            widthDirection = 2;
                            hipWidth = pelvisSize.z;
                            shoulderWidth = shoulderSize.z;
                            baseWidth = pelvisSize.z * 0.5f;

                            if (pelvisSize.y < baseWidth) { pelvisSize.y = baseWidth; }
                            if (pelvisSize.x < baseWidth) { pelvisSize.x = baseWidth; }
                        }
                        
                        //6. From the base size, find if the hips or the last spine segment are closer. Scale up towards that size based on how many steps in the hierarchy it is, away from 
                        //   that source object.
                        int spineLength = lastSpineGroupID - hipGroupID;
                        int scaleToHips = spineLength / 2;

                        float hipScalar = 1f / (scaleToHips + 1);
                        for(int i = 0; i <= scaleToHips; i++)
                        {
                            
                            if(i == 0)
                            {
                                BoxCollider hipBox = (BoxCollider)physicalAnimator.physicsBones[axialGroup.boneIDs[hipGroupID]].collider;
                                hipBox.size = pelvisSize;
                                continue;
                            }


                            int id = axialGroup.boneIDs[i + hipGroupID];
                            
                            
                            BoxCollider spineBox = (BoxCollider)physicalAnimator.physicsBones[id].collider;
                            Vector3 spineSize = spineBox.size;
                            float spineScalar = i * hipScalar;

                            switch (widthDirection)
                            {
                                case 0:
                                    spineSize.x = hipWidth - (hipWidth*spineScalar);
                                    if(spineSize.y < spineSize.z)
                                    {
                                        spineSize.y = baseWidth;
                                    }else
                                    {
                                        spineSize.z = baseWidth;
                                    }
                                    break;

                                case 1:
                                    spineSize.y = hipWidth - (hipWidth*spineScalar);
                                    if (spineSize.x < spineSize.z)
                                    {
                                        spineSize.x = baseWidth;
                                    }
                                    else
                                    {
                                        spineSize.z = baseWidth;
                                    }
                                    break;
                                default:
                                    spineSize.z = hipWidth - (hipWidth * spineScalar);
                                    if (spineSize.y < spineSize.x)
                                    {
                                        spineSize.y = baseWidth;
                                    }
                                    else
                                    {
                                        spineSize.x = baseWidth;
                                    }
                                    break;
                            }
                            spineBox.size = spineSize;
                        }


                        int endCount = spineLength - scaleToHips;

                        for (int i = 1; i < endCount; i++)
                        {
                            int id = axialGroup.boneIDs[i + hipGroupID + scaleToHips];

                           
                            BoxCollider spineBox = (BoxCollider)physicalAnimator.physicsBones[id].collider;
                            Vector3 spineSize = spineBox.size;
                            float spineScalar = i * hipScalar;

                            switch (widthDirection)
                            {
                                case 0:
                                    spineSize.x = (shoulderWidth * spineScalar);
                                    if (spineSize.y < spineSize.z)
                                    {
                                        spineSize.y = baseWidth;
                                    }
                                    else
                                    {
                                        spineSize.z = baseWidth;
                                    }
                                    break;

                                case 1:
                                    spineSize.y = (shoulderWidth * spineScalar);
                                    if (spineSize.x < spineSize.z)
                                    {
                                        spineSize.x = baseWidth;
                                    }
                                    else
                                    {
                                        spineSize.z = baseWidth;
                                    }
                                    break;
                                default:
                                    spineSize.z = (shoulderWidth * spineScalar);
                                    if (spineSize.y < spineSize.x)
                                    {
                                        spineSize.y = baseWidth;
                                    }
                                    else
                                    {
                                        spineSize.x = baseWidth;
                                    }
                                    break;
                            }
                            spineBox.size = spineSize;
                        }

                        switch (widthDirection)
                        {
                            case 0:
                                shoulderSize.x *= 0.85f;
                                if (shoulderSize.y < shoulderSize.z)
                                {
                                    shoulderSize.y = baseWidth;
                                }
                                else
                                {
                                    shoulderSize.z = baseWidth;
                                }
                                break;

                            case 1:
                                shoulderSize.y *= 0.85f;
                                if (shoulderSize.x < shoulderSize.z)
                                {
                                    shoulderSize.x = baseWidth;
                                }
                                else
                                {
                                    shoulderSize.z = baseWidth;
                                }
                                break;
                            default:
                                shoulderSize.z *= 0.85f;
                                if (shoulderSize.y < shoulderSize.x)
                                {
                                    shoulderSize.y = baseWidth;
                                }
                                else
                                {
                                    shoulderSize.x = baseWidth;
                                }
                                break;
                        }
                        int lid = axialGroup.boneIDs[lastSpineGroupID];
                        BoxCollider sb = (BoxCollider)physicalAnimator.physicsBones[lid].collider;
                        sb.size = shoulderSize;
                    }



                    int handGroupID = animationBehaviour.HasGroup("hand");
                    if (handGroupID >= 0)
                    {
                        AnimationGroup handGroup = animationBehaviour.groups[handGroupID];

                        PhysicsBone leftHandBone = physicalAnimator.physicsBones[handGroup.boneIDs[0]],
                                    rightHandBone = physicalAnimator.physicsBones[handGroup.boneIDs[1]];


                        leftHandController.physicalHand = leftHandBone.body;
                        physicalAnimator.ChangeBoneCollider(handGroup.boneIDs[0], "Box");
                        physicalAnimator.ChangeBoneCollider(handGroup.boneIDs[1], "Box");

                        JointSensor leftDropSensor = leftHandBone.transform.gameObject.AddComponent<JointSensor>();
                        //leftHand.dropSensor = leftDropSensor;


                        rightHandController.physicalHand = rightHandBone.body;
                        

                        JointSensor rightDropSensor = rightHandBone.transform.gameObject.AddComponent<JointSensor>();
                        //rightHand.dropSensor = rightDropSensor;

                    }


                    int footGroupID = animationBehaviour.HasGroup("foot");
                    if (footGroupID >= 0)
                    {
                        AnimationGroup footGroup = animationBehaviour.groups[footGroupID];
                        
                        //physicalAnimator.ChangeBoneCollider(footGroup.boneIDs[0], "Box");
                        physicalAnimator.physicsBones[footGroup.boneIDs[0]].AnimationMode = PhysicsAnimationMode.Synchronized;

                        //physicalAnimator.ChangeBoneCollider(footGroup.boneIDs[1], "Box");
                        physicalAnimator.physicsBones[footGroup.boneIDs[1]].AnimationMode = PhysicsAnimationMode.Synchronized;
                    }

                    int ballGroupID = animationBehaviour.HasGroup("ball");
                    if (ballGroupID >= 0)
                    {
                        AnimationGroup ballGroup = animationBehaviour.groups[ballGroupID];

                        physicalAnimator.ChangeBoneCollider(ballGroup.boneIDs[0], "Box");

                        BoxCollider ballCollider = (BoxCollider)physicalAnimator.physicsBones[ballGroup.boneIDs[0]].collider;
                        Vector3 ballSize = ballCollider.size;
                        ballSize.z *= 0.5f;
                        ballCollider.size = ballSize;
                        Vector3 ballOffset = ballCollider.center; ballOffset.z = ballCollider.size.z * -0.5f;
                        ballCollider.center = ballOffset;

                        physicalAnimator.ChangeBoneCollider(ballGroup.boneIDs[1], "Box");
                        ballCollider = (BoxCollider)physicalAnimator.physicsBones[ballGroup.boneIDs[1]].collider;
                        ballSize = ballCollider.size;
                        ballSize.z *= 0.5f;
                        ballCollider.size = ballSize;
                        ballOffset = ballCollider.center; ballOffset.z = ballCollider.size.z * -0.5f;
                        ballCollider.center = ballOffset;
                    }

                    
                   
                    int headID = animationBehaviour.FindBones(headTerms)[0];
                    if(headID >= 0)
                    {
                        physicalAnimator.ChangeBoneCollider(headID, "Sphere"); 
                    }
                    

                    int[] spineIDs = animationBehaviour.FindBones(new AnimationRiggingProfile.ProfileTerms()
                    { 
                        acceptedTerms = new string[] { "spine" },
                        acceptedTermTypes = new AnimationRiggingProfile.TermSetType[] { AnimationRiggingProfile.TermSetType.None } 
                    });

                    if (spineIDs.Length >0)
                    {
                        for (int sp = 0; sp < spineIDs.Length; sp++)
                        {
                            int spid = spineIDs[sp];

                            //CapsuleCollider spineCapsule = (CapsuleCollider)physicalAnimator.physicsBones[spid].collider;
                            //spineCapsule.radius *= 1.25f;
                            physicalAnimator.ChangeBoneJoint(spid, "Ball");

                        }
                        
                    }
                    physicalAnimator.ResetTotalMass();
                    physicalAnimator.SetJointValuesFromHierarchyDepth();
                }
            }

        }
    }
}