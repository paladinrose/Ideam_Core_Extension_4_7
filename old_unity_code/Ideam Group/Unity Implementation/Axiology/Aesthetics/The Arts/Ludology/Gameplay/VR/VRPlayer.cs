using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.Animations.Rigging;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls;

#if UNITY_EDITOR
using UnityEditor;
#endif

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{
    public class VRPlayer : UnityGamePlayer
    {
        [Tooltip("The root transform to be manipulated by the vr player's actions")]
        public Transform controllerRoot;
        
        public VRGhost vrGhost;
        protected TwoHandedAgent twoHanderToPossess;
        
        public VRController characterController;

        #region Ghost Agent Management
        
        public override void SetGhostAgent(GhostAgent newGhostAgent)
        {
            base.SetGhostAgent(newGhostAgent);
            if (newGhostAgent is VRGhost newVRGhost) { vrGhost = newVRGhost; }
        }

        public override void RemoveGhostAgent()
        {
            base.RemoveGhostAgent();
            if (vrGhost)
            {

            }
        }

        #endregion

        #region Possession Management
        
        public override int TakeControlOfAgent(UnityGameAgent toPossess)
        {
            if (toPossess is TwoHandedAgent tha)
            {

                twoHanderToPossess = tha;
               
                return base.TakeControlOfAgent(toPossess);
            }
            return -1;
        }

        protected override void ControlTakenOverAgent()
        {
            base.ControlTakenOverAgent();
        }

        protected override IEnumerator TakingAgentControl()
        {
            token = toPossess;
            for (int i = twoHanderToPossess.rigBuilder.layers.Count - 1; i >= 0; i--)
            {
                RigLayer rigLayer = twoHanderToPossess.rigBuilder.layers[i];
                rigLayer.active = false;
            }

            while (!characterController.headController.HeadIsReady) { yield return null; }

            transform.SetParent(toPossess.AgentRoot);


            float desiredYScale = controllerRoot.localScale.y * twoHanderToPossess.eyelineHeight / characterController.StandingHeight;

            float percentScalar = controllerRoot.localScale.y / desiredYScale;

            Vector3 startingScale = controllerRoot.localScale,
                startingEulerAngles = transform.localEulerAngles,
                startingPosition = transform.localPosition,

                desiredScale = new Vector3(
                controllerRoot.localScale.x * percentScalar,
                desiredYScale,
                controllerRoot.localScale.z * percentScalar
                ),

                desiredEulerAngles = Vector3.zero,
                desiredPosition = Vector3.zero;

            float currentTime = 0, cS = 1f / possessionTimer;

            while (currentTime < possessionTimer)
            {
                float percent = currentTime * cS;

                Vector3 locAng = Vector3.zero;
                locAng.x = Mathf.LerpAngle(startingEulerAngles.x, desiredEulerAngles.x, percent);
                locAng.y = Mathf.LerpAngle(startingEulerAngles.y, desiredEulerAngles.y, percent);
                locAng.z = Mathf.LerpAngle(startingEulerAngles.z, desiredEulerAngles.z, percent);

                controllerRoot.localScale = Vector3.Lerp(startingScale, desiredScale, percent);
                transform.localEulerAngles = locAng;
                transform.localPosition = Vector3.Lerp(startingPosition, desiredPosition, percent);

                currentTime += Time.deltaTime;
                yield return null;
            }

            controllerRoot.localScale = desiredScale;
            transform.localPosition = desiredPosition;
            transform.localEulerAngles = desiredEulerAngles;

            //OK!! Instead of reassigning any targets, I'm going to reparent targets.
            //In this case, the possessed character's human animation rig, and hands rig need to
            //have their hand IK Targets parented to the ghosts targets. And the head IK target, likewise.
            //

            ControlTakenOverAgent();
            yield break;
        }

        public override bool ReleaseAgent(int agentID)
        {
            return base.ReleaseAgent(agentID);
        }

        protected override void AgentReleased()
        {
            base.AgentReleased();
        }
        
        #endregion

        #region MonoBehaviour Methods

        protected override void Awake()
        {
            base.Awake();
            
            //if (vrGhost && characterController.headController) { vrGhost.mappedHeadController = characterController.headController; }

        }
        protected virtual void Start()
        {
            if (vrGhost)
            {
                //if (characterController.leftHandController) { vrGhost.MapToLeftVRHand(characterController.leftHandController); }
                //if (characterController.rightHandController) { vrGhost.MapToRightVRHand(characterController.rightHandController); }
            }
        }

        protected override void OnDestroy()
        {
            base.OnDestroy();
        }

        protected override void OnEnable()
        {
        }

        protected override void OnDisable()
        {
        }

        #endregion


        public static VRPlayer NewVRPlayer(Animator animator = null)
        {
            Transform oldParent = null;
            string baseName = "New VR Player";

            if (animator != null)
            {

                animator.updateMode = AnimatorUpdateMode.AnimatePhysics;
                animator.cullingMode = AnimatorCullingMode.AlwaysAnimate;
                animator.applyRootMotion = false;
                oldParent = animator.transform.parent;
                baseName = animator.name + " (VR Player)";
            }


            GameObject playerObject = new GameObject(baseName);

            playerObject.transform.SetParent(oldParent);
            
            if (animator != null)
            {
                playerObject.transform.position = animator.transform.position;
                playerObject.transform.eulerAngles = animator.transform.eulerAngles;

                animator.transform.SetParent(playerObject.transform, true);
            }

            VRPlayer player = AddVRPlayer(playerObject);

            return player;
        }

        public static VRPlayer AddVRPlayer(GameObject playerObject)
        {
            VRPlayer player = playerObject.AddComponent<VRPlayer>();
            player.possessionTimer = player.endPossessionTimer = 0.1f;
            player.controllerRoot = playerObject.transform;

            return player;
        }
    }
}