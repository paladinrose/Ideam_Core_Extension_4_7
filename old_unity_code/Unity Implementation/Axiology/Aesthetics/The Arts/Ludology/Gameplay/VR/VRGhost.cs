using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Metaphysics.Ontology.Action;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;
using Ideam.Unity.Science.Natural.Physics;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.VR
{
    public class VRGhost : GhostAgent
    {
        public enum GhostingType
        {
            DisableOnPossess,
            Color32,
            Color
        }

        public enum VRGhostingState
        {
            NotGhosted,
            SemiGhosted,
            FullyGhosted
        }

        public TwoHandedAgent possessedTwoHander;
        public HumanoidAgent possessedHumanoid;

        public AnimationBehaviour ghostHandRig;
        
        public PhysicalAnimator ghostPhysics;

        

        protected override void Awake()
        {
            base.Awake();

            //if (ghostPhysics) { StartCoroutine(WaitAndEnableGhostPhysics()); }
        }


        protected virtual void LateUpdate()
        {
            if(ghostHandRig)ghostHandRig.UpdateRigControls();
        }

        
        protected virtual IEnumerator WaitAndEnableGhostPhysics()
        {
            yield return new WaitForSeconds(1f);
            ghostPhysics.PhysicsEnabled = true;
        }

        public override void SetPossessedAgent(UnityGameAgent toPossess)
        {
            if(toPossess is TwoHandedAgent twoPossess)
            {
                if(ghostPhysics)ghostPhysics.PhysicsEnabled = false;   

                base.SetPossessedAgent(toPossess);
                possessedTwoHander = twoPossess;

                //possessedTwoHander.handsRig.HandControl(0).Lock(mappedLeftHand.transform);
                //possessedTwoHander.handsRig.HandControl(1).Lock(mappedRightHand.transform);

                if(toPossess is HumanoidAgent humanoidToPossess)
                {
                    possessedHumanoid = humanoidToPossess;
                    //possessedHumanoid.humanoidRig.rigging[0].controls[3].Lock(mappedHeadController.transform);
                }
            }
        }

        public override void RemovePossessedAgent()
        {
            base.RemovePossessedAgent();
            if (possessedTwoHander)
            {
                possessedTwoHander.handsRig.HandControl(0).ReleaseLock();
                possessedTwoHander.handsRig.HandControl(1).ReleaseLock();
                possessedTwoHander = null;

                if (possessedHumanoid)
                {
                    possessedHumanoid.humanoidRig.rigging[0].controls[3].ReleaseLock();
                    possessedHumanoid = null;
                }

                if(ghostPhysics)ghostPhysics.PhysicsEnabled = true;
            }
        }


        public static VRGhost NewVRGhost(Transform parent = null, Animator animator = null, Rigidbody rigidbody = null, Transform agentRoot = null, Transform mindRoot = null, UnityGame game = null)
        {
            GameObject ghostObject = new GameObject("Ghost Agent");

            if(parent != null)
            {
                ghostObject.transform.SetParent(parent, false);
                ghostObject.transform.localEulerAngles = ghostObject.transform.localPosition = Vector3.zero;

            }

            VRGhost agent = AddVRGhost(ghostObject, animator, rigidbody, agentRoot, mindRoot, game);

            return agent;
        }

        public static VRGhost AddVRGhost(GameObject ghostObject, Animator animator = null, Rigidbody rigidbody = null, Transform agentRoot = null, Transform mindRoot = null, UnityGame game = null)
        {
            VRGhost agent = ghostObject.AddComponent<VRGhost>();

            agent.agentBody = rigidbody;
            agent.agentAnimator = animator;
            agent.AgentRoot = agentRoot;
            
            agent.SetupAsGhost(game, mindRoot);
            
            AddDefaultActions(agent);
            List<UnityGameAction> newDefaultActions = new List<UnityGameAction>(agent.actions);

            //Grab Left
            newDefaultActions.Add(new UnityGameAction()
            {
                actionType = ActionType.Singular,
                sequence = new GamePieceUseData[]
                {
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1 },
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = 1 , sequenceTime = 5f, updateActionValue = true},
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1}
                },
                sequenceEvents = new GameActionEvent[3],
                name = "Grab Left"
            });

            newDefaultActions.Add(new UnityGameAction()
            {
                actionType = ActionType.Singular,
                sequence = new GamePieceUseData[]
                {
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1 },
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = 1 , sequenceTime = 5f, updateActionValue = true},
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1}
                },
                sequenceEvents = new GameActionEvent[3],
                name = "Grab Right"
            });

            agent.actions = newDefaultActions.ToArray();
            return agent;
        }
    }
}