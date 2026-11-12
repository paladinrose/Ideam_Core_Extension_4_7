using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.InputSystem;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.FirstPerson;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class FirstPersonController : BaseController
    {

        #region Properties and Variables
        public Transform headTransform;

        public float walkSpeed, runSpeed, crouchSpeed;
        public Vector2 lookSpeed = Vector2.one;

        [SerializeField]
        protected float standingHeight = 1.8f;
        public float StandingHeight
        {
            get { return standingHeight; }
            set
            {
                if (value == standingHeight) { return; }
                UpdateStandingHeight(value);
            }
        }

        [SerializeField]
        protected float crouchingHeight = 1f;
        public float CrouchingHeight
        {
            get { return crouchingHeight; }
            set
            {
                if (crouching) { }
                crouchingHeight = value;
            }
        }

        public InputActionReference runAction, crouchAction, lookAction;
        
        protected bool running, crouching;

        public FirstPersonUI fpui;

        Quaternion originalRotation;
        #endregion

        protected override void SetupController()
        {
            base.SetupController();
            
            originalRotation = headTransform.localRotation;

            if (runAction) 
            {
                runAction.action.Enable();
                runAction.action.started += evt => { Run(true); };
                runAction.action.canceled += evt => { Run(false); };
            }

            if (crouchAction)
            {
                crouchAction.action.Enable();
                crouchAction.action.started += evt => { Crouch(true); };
                crouchAction.action.canceled += evt => { Crouch(false); };
            }

            if (lookAction && headTransform)
            {
                lookAction.action.Enable();
            }
        }

        protected virtual void UpdateStandingHeight(float newStandingHeight)
        {
            standingHeight = newStandingHeight;
        }

        protected override void UpdateControls()
        {
            base.UpdateControls();

            if(lookAction != null) LookUpdate();
        }
        
        public virtual void LookUpdate()
        {

            Vector2 lookValue = lookAction.action.ReadValue<Vector2>(); //*Time.fixedDeltaTime;

            float rotAverageY = lookValue.y * lookSpeed.y;
            float rotAverageX = lookValue.x * lookSpeed.x;
            //Get the rotation you will be at next as a Quaternion
            Quaternion yQuaternion = Quaternion.AngleAxis(rotAverageY, Vector3.left);
            Quaternion xQuaternion = Quaternion.AngleAxis(rotAverageX, Vector3.up);

            //Rotate
            headTransform.localRotation = originalRotation * xQuaternion * yQuaternion;
        }

        public virtual void Run(bool doRun)
        {
            if (crouching) { running = false; moveControl.Speed = crouchSpeed; return; }

            running = doRun;
            if (running) { moveControl.Speed = runSpeed; }
            else  { moveControl.Speed = walkSpeed; }
        }

        public virtual void Crouch(bool doCrouch)
        {
            crouching = doCrouch;
            if (crouching)
            {
                running = false;
                moveControl.Speed = crouchSpeed;
            }
            else
            {
                moveControl.Speed = walkSpeed;
            }

        }

        public static FirstPersonController NewFirstPersonController(Transform parent)
        {
            string baseName = "New First Person Controller";

            GameObject controllerObject = new GameObject(baseName);

            if (parent != null)
            {
                controllerObject.transform.SetParent(parent);
                controllerObject.transform.localEulerAngles = controllerObject.transform.localPosition = Vector3.zero;
            }

            FirstPersonController controller = AddFirstPersonController(controllerObject);

            return controller;
        }

        public static FirstPersonController AddFirstPersonController(GameObject controllerObject, UnityGameAgent agent = null,
            Rigidbody rigidbody = null)
        {
            FirstPersonController controller = controllerObject.GetComponent<FirstPersonController>();
            if (controller == null)
            {
                controller = controllerObject.AddComponent<FirstPersonController>();
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

                //Ok, what's this still here for? Check in with VRController.
                RaySensor mindRays = null;
                if (agent) { mindRays = agent.mind.sight.linesOfSight; }

                //This was for hands. We're not using hands. How are we handling clings?

                List<ClingControl.ClingData> cd = new List<ClingControl.ClingData>();
               

                controller.clingData = cd.ToArray();
                controller.clingControl.clingData = cd.ToArray();

            }

            return controller;
        }
    }
}