using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class VRController : FirstPersonController
    {
		[Header("VR Devices")]
		[Tooltip("The head mounted display")]
		public VRHeadController headController;
        public VRUI vrui;

		[Tooltip("The left hand controller or hand-capture")]
		public VRHandController leftHandController;
        public float leftGrabPauseTimeLimit;
        public float leftGrabActionToClingForce;
        protected float leftGrabPauseTime;
        protected Coroutine leftGrabPauseRoutine;

        [Tooltip("The right hand controller or hand-capture")]
		public VRHandController rightHandController;
        public float rightGrabPauseTimeLimit;
        public float rightGrabActionToClingForce;
        protected float rightGrabPauseTime;
        protected Coroutine rightGrabPauseRoutine;

        public bool setAvatarToPlayerHeightOnStart;

        [SerializeField]
        protected float heightOffset;
        protected override void SetupController()
        {
            base.SetupController();

            headController.WhenHeadIsReady += () =>
            {
                if (setAvatarToPlayerHeightOnStart) { standingHeight = headController.BaseVRHeight; }
                UpdateStandingHeight(standingHeight);
            };


            if (clingActions.Length >0)
            {
                if (agent)
                {
                    int leftGrabActionID = agent.GetActionID("Grab Left");

                    if (leftGrabActionID >= 0)
                    {
                        UnityGameAction leftGrabGameAction = agent.actions[leftGrabActionID];

                        //START CLINGING

                        leftGrabGameAction.sequenceEvents[0].OnEvent += evt =>
                        {

                            if (leftGrabPauseTime >= leftGrabPauseTimeLimit)
                            {
                                clingControl.clingData[0].clingBreakForce = evt * leftGrabActionToClingForce;
                            }
                        };

                        //MOVE UPDATE

                        leftGrabGameAction.sequenceEvents[1].OnEvent += evt =>
                        {
                            clingControl.clingData[0].clingBreakForce = evt * leftGrabActionToClingForce;
                        };

                        //STOP MOVING
                       
                        leftGrabGameAction.sequenceEvents[2].OnEvent += evt =>
                        {
                        };
                    }


                    int rightGrabActionID = agent.GetActionID("Grab Right");

                    if (rightGrabActionID >= 0)
                    {
                        UnityGameAction rightGrabGameAction = agent.actions[rightGrabActionID];

                        //START CLINGING
                        
                        rightGrabGameAction.sequenceEvents[0].OnEvent += evt =>
                        {

                            if (rightGrabPauseTime >= rightGrabPauseTimeLimit)
                            {
                                clingControl.clingData[1].clingBreakForce = evt * rightGrabActionToClingForce;
                            }
                        };

                        //CLING UPDATE
                        
                        rightGrabGameAction.sequenceEvents[1].OnEvent += evt =>
                        {
                            clingControl.clingData[1].clingBreakForce = evt * rightGrabActionToClingForce;
                        };

                        //STOP CLINGING
                        
                        rightGrabGameAction.sequenceEvents[2].OnEvent += evt =>
                        { 
                        };
                    }

                    //CLING CONTROL EVENTS

                    clingControl.OnStartClinging += (id, obj) =>
                    {
                        if (id == 0)
                        {
                            if (leftGrabPauseTime >= leftGrabPauseTimeLimit)
                            {
                                agent.StartAction(leftGrabActionID);
                            }
                            else if (leftGrabPauseRoutine != null)
                            {
                                StopCoroutine(leftGrabPauseRoutine);
                                leftGrabPauseRoutine = null;
                            }
                        }
                        else if (id == 1)
                        {
                            if (rightGrabPauseTime >= rightGrabPauseTimeLimit)
                            {
                                agent.StartAction(rightGrabActionID);
                            }
                            else if (rightGrabPauseRoutine != null)
                            {
                                StopCoroutine(rightGrabPauseRoutine);
                                rightGrabPauseRoutine = null;
                            }
                        }
                    };

                    clingControl.OnClingUpdate += evt => 
                    {
                        if (evt == 0) { agent.UpdateAction(leftGrabActionID, Time.deltaTime); }
                        else if (evt == 1) { agent.UpdateAction(rightGrabActionID, Time.deltaTime); }
                    };

                    clingControl.OnStopClinging += (id, obj) =>
                    {
                        if (id == 0) 
                        {
                            leftGrabPauseTime = 0;
                            leftGrabPauseRoutine = StartCoroutine(LeftGrabPause(leftGrabActionID));
                        }
                        else
                        {
                            rightGrabPauseTime = 0;
                            rightGrabPauseRoutine = StartCoroutine(RightGrabPause(rightGrabActionID));
                        }
                    };
                }
            }
        }

        protected override void UpdateStandingHeight(float newStandingHeight)
        {
            standingHeight = newStandingHeight;
            if (!Application.isPlaying) { return; }

            Transform root = headController.controlRoot;
            Vector3 newPos = root.up.normalized;
            heightOffset = standingHeight - headController.BaseVRHeight;
            newPos *= heightOffset;

            if (headController.headControl) { newPos -= headController.headControl.lockSettings.positionOffset; }

            root.position = root.position + headController.VRPositionOffset + newPos;

        }
        
        private IEnumerator LeftGrabPause(int leftGrabActionID)
        {
            while (leftGrabPauseTime < leftGrabPauseTimeLimit) { leftGrabPauseTime += Time.deltaTime; yield return null; }
            agent.CompleteAction(leftGrabActionID);
            yield break;
        }

        private IEnumerator RightGrabPause(int rightGrabActionID)
        {
            while (rightGrabPauseTime < rightGrabPauseTimeLimit) { rightGrabPauseTime += Time.deltaTime; yield return null; }
            agent.CompleteAction(rightGrabActionID);
            yield break;
        }


        public static VRController NewVRController(Transform parent)
        {
            string baseName = "New VR Controller";
            
            GameObject controllerObject = new GameObject(baseName);

            if (parent != null)
            {
                controllerObject.transform.SetParent(parent);
                controllerObject.transform.localEulerAngles = controllerObject.transform.localPosition = Vector3.zero;
            }

            VRController controller = AddVRController(controllerObject);

            return controller;
        }

        public static VRController AddVRController(GameObject controllerObject, UnityGameAgent agent = null, 
            GameObject headObject = null, GameObject leftHandObject = null, GameObject rightHandObject = null, 
            Rigidbody rigidbody = null)
        {
            VRController controller = controllerObject.GetComponent<VRController>();
            if (controller == null)
            {
                controller = controllerObject.AddComponent<VRController>();
                controller.walkSpeed = 1f;
                controller.runSpeed = 1.5f;
                controller.crouchSpeed = 0.5f;
                controller.movePauseTimeLimit = 1f;
                controller.moveActionToSpeed = 1f;


                controller.moveControl = controllerObject.AddComponent<MoveControl>();
                controller.moveControl.Speed = 1f;
                controller.turnControl = controllerObject.AddComponent<TurnControl>();
                controller.turnControl.turnSpeed = 120f;
                controller.pushControl = controllerObject.AddComponent<PushControl>();
                controller.clingControl = controllerObject.AddComponent<ClingControl>();
                controller.animatorControl = controllerObject.AddComponent<AnimatorControl>();


                controller.moveControl.controlTarget = controller.turnControl.controlTarget = controller.pushControl.controlTarget = controller.clingControl.controlTarget = controllerObject.transform;

                controller.moveControl.controller = controller.turnControl.controller = controller.pushControl.controller = controller.clingControl.controller = controller;

                controller.moveControl.controlBody = controller.turnControl.controlBody = controller.pushControl.controlBody = controller.clingControl.controlBody = rigidbody;


                PushControl.PushData jumpData = new PushControl.PushData()
                {
                    pusher = controllerObject.transform,
                    pusherBody = rigidbody,
                    requirePushTarget = false,
                    pushForce = new Vector3(0, 20f, 0)
                };
                controller.pushData = new PushControl.PushData[] { jumpData };
                controller.pushControl.pushData = new PushControl.PushData[] { jumpData };

                controller.StandingHeight = 1.7f;
                controller.CrouchingHeight = 1.15f;
                RaySensor mindRays = null;
                if (agent) { mindRays = agent.mind.sight.linesOfSight; }

                if (headObject)
                {

                    VRHeadController headController = VRHeadController.AddVRHeadController(headObject, controllerObject.transform, mindRays);
                    controller.headController = headController;
                }

                List<ClingControl.ClingData> cd = new List<ClingControl.ClingData>();
                if (leftHandObject)
                {
                    VRHandController leftHandController = VRHandController.AddHandController(leftHandObject,controllerObject.transform, true, mindRays);
                    
                    controller.leftHandController = leftHandController;
                    
                    controller.leftGrabActionToClingForce = 1f;
                    controller.leftGrabPauseTimeLimit = 1f;
                    
                    ClingControl.ClingData grabLeftData = new ClingControl.ClingData()
                    {
                        clinger = leftHandController.transform,
                        clingSensor = leftHandController.palmSensor,
                        clingerBody = leftHandController.physicalHand

                    };
                    cd.Add(grabLeftData);
                }

                if (rightHandObject)
                {
                    VRHandController rightHandController = VRHandController.AddHandController(rightHandObject, controllerObject.transform, false, mindRays);
                    
                    controller.rightHandController = rightHandController;

                    controller.rightGrabActionToClingForce = 1f;
                    controller.rightGrabPauseTimeLimit = 1f;

                    ClingControl.ClingData grabRightData = new ClingControl.ClingData()
                    {
                        clinger = rightHandController.transform,
                        clingSensor = rightHandController.palmSensor,
                        clingerBody = rightHandController.physicalHand
                    };
                    cd.Add(grabRightData);
                } 
                
                controller.clingData = cd.ToArray();
                controller.clingControl.clingData = cd.ToArray();
            
            }

            return controller;
        }
    }
}