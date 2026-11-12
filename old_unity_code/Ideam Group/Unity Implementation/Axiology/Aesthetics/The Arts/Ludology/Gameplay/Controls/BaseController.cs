using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;

using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Metaphysics.Ontology.Action;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    public class BaseController : MonoBehaviour
    {
        public UnityGameAgent agent;
        
        public AnimatorControl animatorControl;

        public MoveControl moveControl;
        public InputActionReference moveAction;
        public float movePauseTime, movePauseTimeLimit, moveActionToSpeed;

        public TurnControl turnControl;
        public InputActionReference turnAction;

        public ClingControl clingControl;
        public ClingControl.ClingData[] clingData = new ClingControl.ClingData[0];
        public InputActionReference[] clingActions;
        protected int[] clingControlIDs;

        public PushControl pushControl;
        public PushControl.PushData[] pushData;
        public InputActionReference[] pushActions;

        protected Coroutine movePauseRoutine;
        

        private void Awake()
        {
            SetupController();
        }

        private void FixedUpdate()
        {
            UpdateControls();
        }

        
        protected virtual void SetupController()
        {
            if (moveAction) 
            {
                moveAction.action.Enable(); 
                if (agent) 
                {
                    int moveActionID = agent.GetActionID("Move");
                    
                    if (moveActionID >= 0)
                    {
                        UnityGameAction moveGameAction = agent.actions[moveActionID];

                        //START MOVING
                        moveControl.OnStartMoving += () => 
                        {
                            if (movePauseTime >= movePauseTimeLimit)
                            {
                                agent.StartAction(moveActionID);
                            }
                            else if(movePauseRoutine != null)
                            {
                                StopCoroutine(movePauseRoutine);
                                movePauseRoutine = null;
                            }
                        };

                        moveGameAction.sequenceEvents[0].OnEvent += evt => 
                        {
                            //Debug.Log("On Start Moving (Called from sequence event)");
                            if (movePauseTime >= movePauseTimeLimit) 
                            {
                                //evt is currentActionValue. How do I convert that to speed? Where do I store that conversion?
                                moveControl.Speed = evt * moveActionToSpeed;
                            }
                        };

                        //MOVE UPDATE
                        moveControl.OnMove += evt => { agent.UpdateAction(moveActionID, Time.deltaTime); };
                        
                        moveGameAction.sequenceEvents[1].OnEvent += evt => 
                        {
                            //Debug.Log("Move Update (Called from sequence event)");
                            moveControl.Speed = evt * moveActionToSpeed;
                        };

                        //STOP MOVING
                        moveControl.OnStopMoving += () => 
                        {
                            movePauseTime = 0;
                            movePauseRoutine = StartCoroutine(MovePause(moveActionID));
                             
                        };

                        moveGameAction.sequenceEvents[2].OnEvent += evt =>
                        {
                            //Debug.Log("On Stop Moving (Called from sequence event)");
                        };
                    }
                } 
            }
            if (turnAction) { turnAction.action.Enable(); }

            for(int i =0; i < pushActions.Length; i++)
            {
                int pid = i;
                if (pushActions[i]) 
                { 
                    pushActions[i].action.Enable();
                    pushData[i].collisionSensor.OnFirstCollisionStart.sensorEvent += evt => { OnSomethingToPush(pid, evt.collision.gameObject); };
                    pushData[i].collisionSensor.OnLastCollisionStop.sensorEvent += evt => { OnNothingToPush(pid, evt.collision.gameObject); };
                }
            }

            clingControlIDs = new int[clingActions.Length];
            for (int i = 0; i < clingActions.Length; i++)
            {
                int grabID = i;
                if (clingActions[i])
                {
                    clingActions[i].action.Enable();
                    
                    clingActions[i].action.started += evt => { StartCling(grabID); };
                    clingActions[i].action.canceled += evt => { StopCling(grabID); };
                    TriggerSensor grabSensor = clingData[grabID].clingSensor;

                    grabSensor.OnTriggerStart.hitEvent += evt => { OnSenseGrabbable(grabID, evt); };
                    grabSensor.OnTriggerStop.hitEvent += evt => { OnStopSensingGrabbable(grabID, evt); };
                    
                }
            }
        }

        

        private IEnumerator MovePause(int moveActionID)
        {
            while(movePauseTime < movePauseTimeLimit) { movePauseTime += Time.deltaTime; yield return null; }
            agent.CompleteAction(moveActionID);
            yield break;
        }


        protected virtual void UpdateControls()
        {
            //Gather input from analogue inputs.
            if (moveControl)  {  Move(); }
            if (turnControl)  { Turn(); }
            if (animatorControl) { Animate(); }
            if (clingControl)
            {
                for(int i =0; i < clingActions.Length; i++)
                {
                    if (!clingActions[i].action.enabled || !clingData[i].isClinging) { continue; }
                    Cling(i);

                }
            }
            //Update controls
            if (clingControl) { clingControl.ClingUpdate(); }
            if (pushControl) { pushControl.PushUpdate(); }
            if (turnControl) { turnControl.UpdateTurnControl(); }
            if (moveControl) { moveControl.UpdateMoveControl(); }

            if(animatorControl) { animatorControl.UpdateBlendShapes(); }
        }

        #region Moving

        protected virtual void Move()
        {
            if (!moveAction) { return; }
            Vector3 moveBy = moveAction.action.ReadValue<Vector2>();
            moveBy.z = moveBy.y;
            moveBy.y = 0;

            moveControl.Move(moveBy);
            
        }

        #endregion

        #region Turning

        protected virtual void Turn()
        {
            if (!turnAction) return;
            Vector3 turnBy = new Vector3(0, turnAction.action.ReadValue<Vector2>().x, 0);
            turnControl.Turn(turnBy);
            
        }

        #endregion

        #region Pushing

        protected virtual void Push(int pushID)
        {
            var pd = new PushControl.PushData(pushData[pushID]);
            pushControl.Push(pd);
        }

        protected virtual void OnNothingToPush(int pushID, GameObject lastToEndContact)
        {
            var pd = pushData[pushID];
            pd.pushTarget = null;
            pushData[pushID] = pd;
        }

        protected virtual void OnSomethingToPush(int pushID, GameObject inContact)
        {
            var pd = pushData[pushID];
            pd.pushTarget = inContact.transform;
            pushData[pushID] = pd;
        }


        #endregion

        #region Grabbing

        protected virtual void StartCling(int clingID)
        {
            ClingControl.ClingData cling = clingData[clingID];
            cling.isClinging = true;
            
            clingControlIDs[clingID] = clingControl.StartCling(cling);
            
            clingData[clingID] = cling;
        }

        protected virtual void Cling(int clingID)
        {
            
            ClingControl.ClingData cling = clingData[clingID]; 
            cling.clingStrength = clingActions[clingID].action.ReadValue<float>();

            clingData[clingID] = cling;
            clingControl.UpdateClingValues(clingControlIDs[clingID], cling.clingStrength);
        }

        protected virtual void StopCling(int clingID)
        {

            ClingControl.ClingData cling = clingData[clingID];
            cling.clingStrength = 0f; cling.isClinging = false;
            cling.clingTarget = null;
            clingData[clingID] = cling;
            clingControl.StopClinging(clingControlIDs[clingID]);
        }


        protected virtual void OnSenseGrabbable(int grabID, GameObject grabbableObject)
        {
            if(grabID >= clingData.Length ) { return; }
            
            ClingControl.ClingData cling = clingData[grabID];
            cling.clingTarget = grabbableObject.transform;
            clingData[grabID] = cling;
        }

        protected virtual void OnStopSensingGrabbable(int grabID, GameObject grabbableObject)
        {
            if (grabID >= clingData.Length) { return; }

            ClingControl.ClingData cling = clingData[grabID];
            cling.clingTarget = null;
            clingData[grabID] = cling;
        }


        #endregion

        #region Animate

        protected virtual void Animate()
        {

        }

        #endregion

        protected static void AddDefaultControls(GameObject controllerObject, BaseController controller, Rigidbody rigidbody = null)
        {
            

            controller.moveControl = controllerObject.AddComponent<MoveControl>();
            controller.turnControl = controllerObject.AddComponent<TurnControl>();
            controller.pushControl = controllerObject.AddComponent<PushControl>();

            controller.moveControl.controlTarget = controller.turnControl.controlTarget = controller.pushControl.controlTarget = controllerObject.transform;
            controller.moveControl.controller = controller.turnControl.controller = controller.pushControl.controller = controller;
            controller.moveControl.controlBody = controller.turnControl.controlBody = controller.pushControl.controlBody = rigidbody;

            PushControl.PushData jumpData = new PushControl.PushData()
            {
                pusher = controllerObject.transform,
                pusherBody = rigidbody,
                requirePushTarget = false,
                pushForce = new Vector3(0, 20f, 0)
            };
            controller.pushData = new PushControl.PushData[] { jumpData };
        }

    }
}