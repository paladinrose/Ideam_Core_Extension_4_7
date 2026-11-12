using System;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.Events;

using Ideam.Science.Formal.Mathematics;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Metaphysics.Ontology.Action;
using Ideam.Metaphysics.Mind;

using Ideam.Unity.Metaphysics.Mind;

using Quaternion = UnityEngine.Quaternion;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{ 
    public class UnityGameAgent : MonoBehaviour, IAgent
    {
        public Animator agentAnimator;
        public Rigidbody agentBody;

        protected virtual void Awake()
        {
            
        }

        public virtual void SetupAsGhost(UnityGame game = null, Transform mindRoot = null)
        {
            this.game = game;

            GameObject mindObject = gameObject;
            
            if (mindRoot)
            {
                mindObject = mindRoot.gameObject;
            }

            mind = UnityMind.AddUnityMind(mindObject);
            mind.agent = this;

            SpiritForm spiritForm = gameObject.AddComponent<SpiritForm>();
            System.Array.Resize(ref gamePieces, gamePieces.Length + 1);
            gamePieces[gamePieces.Length - 1] = spiritForm;

            spiritForm.gameAgent = this;
            spiritForm.game = game;

            int physicalitySememe = -1, staminaSememe = -1,
                focusSememe = -1, attentionSememe = -1;

            if (game && game.languageAsset)
            {
                mind.semanticManager = game.languageAsset.semanticManager;
                mind.languages = new UnityLanguageAsset[] { game.languageAsset };

                spiritForm.semanticManager = game.languageAsset.semanticManager;
                spiritForm.languageAsset = game.languageAsset;

                physicalitySememe = game.languageAsset.GetSemanticID("Physicality");
                staminaSememe = game.languageAsset.GetSemanticID("Stamina");

                focusSememe = game.languageAsset.GetSemanticID("Focus");
                attentionSememe = game.languageAsset.GetSemanticID("Attention");
            }

            UnityGamePieceProperty agentPhysicality = new UnityGamePieceProperty() { sememe = physicalitySememe };
            UnityGamePieceProperty agentFocus = new UnityGamePieceProperty() { sememe = focusSememe };
            spiritForm.properties = new UnityGamePieceProperty[] { agentPhysicality, agentFocus };

            UnityGamePieceResource agentStamina = new UnityGamePieceResource() { sememe = staminaSememe };
            UnityGamePieceResource agentAttention = new UnityGamePieceResource() { sememe = attentionSememe };
            spiritForm.resources = new UnityGamePieceResource[] { agentStamina, agentAttention };

        }


        #region IAgent Implementation

        [SerializeField]
        protected Transform agentRoot;
        public virtual Transform AgentRoot { get { if (agentRoot == null) { agentRoot = transform; } return agentRoot; } set { if (value != agentRoot) agentRoot = value; } }

        public event Action<IAction> OnActionStart, OnActionUpdate, OnActionComplete, OnActionFail, OnActionInterrupted;

        public UnityGame game;
        public UnityGameBoard gameBoard;

        public UnityMind mind;
        public UnityGamePiece[] gamePieces = new UnityGamePiece[0];
        public UnityGameAction[] actions = new UnityGameAction[0];

        public UnityGameAction[] inProgressActions = new UnityGameAction[0];
        
        public virtual T GetMind<T>() where T : IMind
        {
            if(mind is T tMind) { return tMind; }
            return default;
        }

        public virtual T[] GamePieces<T>() where T : IThing
        {
            List<T> tPieces = new List<T>();
            for (int i = 0; i < gamePieces.Length; i++)
            {
                if (gamePieces[i] is T tPiece)
                {
                    tPieces.Add(tPiece);
                }
            }
            return tPieces.ToArray();
        }

        public virtual T[] Actions<T>() where T : IAction
        {
            List<T> tActions = new List<T>();
            for (int i = 0; i < actions.Length; i++)
            {
                if (actions[i] is T tAction)
                {
                    tActions.Add(tAction);
                }
            }
            return tActions.ToArray();
        }

        public virtual T[] InProgressActions<T>() where T : IAction
        {
            List<T> tActions = new List<T>();
            for (int i = 0; i < inProgressActions.Length; i++)
            {
                if (inProgressActions[i] is T tAction)
                {
                    tActions.Add(tAction);
                }
            }
            return tActions.ToArray();
        }

        public UnityGameAction GetAction(string actionName)
        {
            for(int i =0; i < actions.Length; i++)
            {
                if (actions[i].name == actionName) { return actions[i]; }
            }
            return null;
        }

        public int GetActionID(string actionName)
        {
            for(int i =0; i < actions.Length; i++)
            {
                if(actions[i].name == actionName) { return i; }
            }
            return -1;
        }

        #endregion

        #region Action and Interaction Management

        public virtual void StartAction(int actionID)
        {
            var act = actions[actionID];
            act.CurrentSequenceID = 0;
            
            //Now, run the starting sequence and add values to V along the way.
            //Call OnActionStart and act.sequenceEvents[0].CallEvent();
            GamePieceUseData currentData = act.sequence[act.CurrentSequenceID];

            UpdateActionValue(act, currentData);
           
            act.sequenceEvents[act.CurrentSequenceID].CallEvent(act.CurrentActionValue);
            
            OnActionStart?.Invoke(actions[actionID]);

            if (currentData.sequenceGoTo >= 0) { act.CurrentSequenceID = currentData.sequenceGoTo; }
            else { act.CurrentSequenceID++; }
        }

        public virtual void UpdateAction(int actionID)
        {
            OnActionUpdate?.Invoke(actions[actionID]);

        }

        public virtual void UpdateAction(int actionID, float deltaTime)
        {
            UnityGameAction act = actions[actionID];

            GamePieceUseData currentData = act.sequence[act.CurrentSequenceID];

            act.currentSequenceTime += deltaTime;

            if(currentData.sequenceTime < 0)
            {
                //We just want to wait on this step until something else moves us on.
                //If I don't end up needing to do any work here, I should ammend the else if to a regular if:
                // if(currentData.sequenceTime > 0 && act.currentSequenceTime >= currentData.sequenceTime)
            }
            else if(act.currentSequenceTime >= currentData.sequenceTime)
            {
                act.currentSequenceTime -= act.sequence[act.CurrentSequenceID].sequenceTime;
                if (currentData.sequenceGoTo >= 0) { act.CurrentSequenceID = currentData.sequenceGoTo; }
                else { act.CurrentSequenceID++; }

                if(act.CurrentSequenceID >= act.sequence.Length)
                {
                    CompleteAction(actionID);
                    return;
                }
                else
                {
                    currentData = act.sequence[act.CurrentSequenceID];
                    if (currentData.updateActionValue) { UpdateActionValue(act, currentData); }
                    act.sequenceEvents[act.CurrentSequenceID].CallEvent(act.CurrentActionValue);
                }
            }

            OnActionUpdate?.Invoke(act);
        }

        public virtual void InterruptAction(int actionID)
        {
            OnActionInterrupted?.Invoke(actions[actionID]);
        }

        public virtual void CompleteAction(int actionID)
        {
            UnityGameAction act = actions[actionID];
            int lc = act.sequence.Length - 1;
            if(act.CurrentSequenceID < lc) 
            {
                act.sequenceEvents[lc].CallEvent(act.CurrentActionValue);
                OnActionUpdate?.Invoke(act);
            }
            OnActionComplete?.Invoke(actions[actionID]);
        }

        public virtual void FailAction(int actionID)
        {
            OnActionFail?.Invoke(actions[actionID]);
        }

        protected virtual void UpdateActionValue(UnityGameAction act, GamePieceUseData currentData)
        {
            Int v = 0;
            if (currentData.pieceID >= 0)
            {
                UnityGamePiece sequencePiece = gamePieces[currentData.pieceID];
                if (currentData.propertyID >= 0)
                {
                    v += sequencePiece.properties[currentData.propertyID].CurrentValue;
                }
                if (currentData.resourceID >= 0)
                {
                    v += sequencePiece.resources[currentData.resourceID].UseResource();
                }
            }

            act.CurrentActionValue = v;
        }

        public virtual void Interact(UnityGameInteraction interaction)
        {

        }

        public void NewActionAttempt(UnityGameAction newAction)
        {
            if (game.ValidateAction(newAction)) 
            {
                //The action gets added to the actions list. Even if the attempt will go on to fail, get interrupted, etc, since it was POSSIBLE,
                //and within the rules, it gets added to the Actions list.

                int l = actions.Length;
                System.Array.Resize(ref actions, l + 1);
                actions[l] = newAction;

                StartAction(l);
            }
            else { }
        }
        #endregion

        public static UnityGameAgent NewUnityGameAgent(Transform parent = null, Animator animator = null, Rigidbody rigidbody = null, UnityGame game = null)
        {
            GameObject agentObject = new GameObject("Game Agent");
            if(animator != null) { agentObject.name = animator.gameObject.name + " (Game Agent)"; }
            if (parent != null)
            {
                agentObject.transform.SetParent(parent, false);
                agentObject.transform.localEulerAngles = agentObject.transform.localPosition = Vector3.zero;

            }

            UnityGameAgent agent = AddUnityGameAgent(agentObject, animator, rigidbody, parent, game);

            return agent;
        }
        
        public static UnityGameAgent AddUnityGameAgent(GameObject agentObject, Animator animator = null, Rigidbody rigidbody = null, Transform agentRoot = null, UnityGame game = null, bool makeMindObject = false)
        {
            UnityGameAgent agent = agentObject.AddComponent<UnityGameAgent>();

            agent.agentBody = rigidbody;
            agent.agentAnimator = animator;
            agent.AgentRoot = agentRoot;
            agent.game = game;

            GameObject mindObject = null;

            if (makeMindObject) 
            { 
                mindObject = new GameObject("Agent Mind"); 
                mindObject.transform.SetParent(agentObject.transform); 
                mindObject.transform.localEulerAngles = mindObject.transform.localPosition = Vector3.zero;
            }
            else { mindObject = agent.gameObject; }
            
            UnityMind agentMind = UnityMind.AddUnityMind(mindObject);
            agent.mind = agentMind;
            
            agentMind.agent = agent;

            UnityGamePiece agentAnima = agentObject.AddComponent<UnityGamePiece>();
            agentAnima.gameAgent = agent;
            agentAnima.game = game;

            int physicalitySememe = -1, staminaSememe = -1,
                focusSememe = -1, attentionSememe = -1;

            if (game && game.languageAsset)
            {
                agentMind.semanticManager = game.languageAsset.semanticManager;
                agentMind.languages = new UnityLanguageAsset[] { game.languageAsset };

                agentAnima.semanticManager = game.languageAsset.semanticManager;
                agentAnima.languageAsset = game.languageAsset;

                physicalitySememe = game.languageAsset.GetSemanticID("Physicality");
                staminaSememe = game.languageAsset.GetSemanticID("Stamina");

                focusSememe = game.languageAsset.GetSemanticID("Focus");
                attentionSememe = game.languageAsset.GetSemanticID("Attention");
            }

            UnityGamePieceProperty agentPhysicality = new UnityGamePieceProperty() { sememe = physicalitySememe };
            UnityGamePieceProperty agentFocus = new UnityGamePieceProperty() { sememe = focusSememe };
            agentAnima.properties = new UnityGamePieceProperty[] { agentPhysicality, agentFocus };

            UnityGamePieceResource agentStamina = new UnityGamePieceResource() { sememe = staminaSememe };
            UnityGamePieceResource agentAttention = new UnityGamePieceResource() { sememe = attentionSememe };
            agentAnima.resources = new UnityGamePieceResource[] { agentStamina,  agentAttention };

            agent.gamePieces = new UnityGamePiece[] { agentAnima };

            AddDefaultActions(agent);
            
            return agent;
        }

        public static void AddDefaultActions(UnityGameAgent agent)
        {
            List<UnityGameAction> newDefaultActions = new List<UnityGameAction>();

            //Walk
            newDefaultActions.Add( new UnityGameAction() 
            {
                actionType = ActionType.Continuous, 
                sequence = new GamePieceUseData[] 
                {
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1 },
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = 1 , sequenceTime = 5f, updateActionValue = true},
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1}
                },
                sequenceEvents = new GameActionEvent[3], 
                name = "Move"
            } );

            //Run
            newDefaultActions.Add( new UnityGameAction()
            {
                actionType = ActionType.Continuous,
                sequence = new GamePieceUseData[]
                {
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1 },
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = 1 , sequenceTime = 2f, updateActionValue = true},
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1}
                },
                sequenceEvents = new GameActionEvent[3],
                name = "Run"
            });

            //Jump
            newDefaultActions.Add( new UnityGameAction()
            {
                actionType = ActionType.Continuous,
                sequence = new GamePieceUseData[]
                {
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1 },
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1 , sequenceTime = -1f, updateActionValue = true},
                    new GamePieceUseData() { propertyID = 0, resourceID = 0, sequenceGoTo = -1}
                },
                sequenceEvents = new GameActionEvent[3],
                name = "Jump"
            });

            if (agent.mind)
            {
                if (agent.mind.sight.linesOfSight)
                {
                    newDefaultActions.Add(new UnityGameAction()
                    {
                        actionType = ActionType.Continuous,
                        sequence = new GamePieceUseData[]
                        {
                            new GamePieceUseData() { propertyID = 1, resourceID = 1, sequenceGoTo = -1 },
                            new GamePieceUseData() { propertyID = 1, resourceID = 1, sequenceGoTo = 1 , sequenceTime = 1f, updateActionValue = true},
                            new GamePieceUseData() { propertyID = 1, resourceID = 1, sequenceGoTo = -1}
                        },
                        sequenceEvents = new GameActionEvent[3],
                        name = "Look"
                    });
                }

                if (agent.mind.hearing.hearing)
                {
                    newDefaultActions.Add(new UnityGameAction()
                    {
                        actionType = ActionType.Continuous,
                        sequence = new GamePieceUseData[]
                        {
                            new GamePieceUseData() { propertyID = 1, resourceID = 1, sequenceGoTo = -1 },
                            new GamePieceUseData() { propertyID = 1, resourceID = 1, sequenceGoTo = 1 , sequenceTime = 1f, updateActionValue = true},
                            new GamePieceUseData() { propertyID = 1, resourceID = 1, sequenceGoTo = -1}
                        },
                        sequenceEvents = new GameActionEvent[3],
                        name = "Listen"
                    });
                }
            }

            

            UnityGameAction[] defaultAgentActions = newDefaultActions.ToArray();

            agent.actions = defaultAgentActions;
        }

        protected virtual void OnDrawGizmos()
        {
        }
    }

    [Serializable]
    public class GameActionEvent
    {
        public UnityEvent<int> actionUnityEvent;

        public event Action<Int> OnEvent;

        public void CallEvent(int currentActionValue)
        {
            actionUnityEvent?.Invoke(currentActionValue);
            OnEvent?.Invoke(currentActionValue);
        }
    }

    [Serializable]
    public class UnityGameAction : IAction
    {
        //Farther down the line, when it becomes relevant, I want to replace or supplement this with semantics.
        //Once this is done, I want to replace GetAction, where we're looking up actions by string and matching to name, by looking up that string, semantically,
        //then finding the action who's semantics most perfectly match, by meeting all the sememes first, then by having JUST those second, with no extra sememes.
        public string name;
 
        [SerializeField]
        protected int currentSequenceID;
        public int CurrentSequenceID
        { 
            get { return currentSequenceID; } 
            set 
            { 
                currentSequenceID = value; 

                if(currentSequenceID >= sequence.Length && actionType == ActionType.Repeated)
                {
                    //Putting this on hold.    
                }
            }
        }

        public GamePieceUseData[] sequence;
        public GamePieceUseData[] Sequence { get { return sequence; } }
        //The length of sequenceEvents needs to match number of sub-sequences. 
        //Each time we update the action, we go through a sub-sequence, update numbers,
        //and run CallEvent on the current sequenceEvent.
        public GameActionEvent[] sequenceEvents;
        
        public ActionType actionType;
        public ActionType ActionType { get { return actionType; } }

        public bool isInteractive;


        [SerializeField]
        protected Int currentActionValue;
        public Int CurrentActionValue { get { return currentActionValue; } set { currentActionValue = value; } }

        public float currentSequenceTime;
        public void SetupUnityGameAction()
        {
            
        }
    }

    [Serializable]
    public class UnityGameInteraction : IInteraction
    {
        

        public ActionType interactionType;

        public virtual event Action OnBeginInteraction, OnEndInteraction;

        public UnityGameAgent[] causalAgents = new UnityGameAgent[0],
                                effectedAgents = new UnityGameAgent[0];

        public UnityGameAction[] causalActions = new UnityGameAction[0];
        public UnityGamePiece[]  causalPieces = new UnityGamePiece[0], 
                                 effectedPieces = new UnityGamePiece[0];
        
        [Serializable]
        public struct InteractionEffect
        {
            public int agentID;
            public List<int> effectedPieces,
                             effectedProperties,
                             effectedResources,
                             motivatedActionIDs;

            public InteractionEffect(int agentID)
            {
                this.agentID = agentID;
                effectedPieces = new List<int>();
                effectedProperties = new List<int>();
                effectedResources = new List<int>();
                motivatedActionIDs = new List<int>();
            }
        }

        public List<InteractionEffect> interactionEffects;

        public virtual void BeginInteraction()
        {
            OnBeginInteraction?.Invoke();
        }

        public virtual T[] CausalAgents<T>() where T : IAgent
        {
            List<T> tEntities = new List<T>();
            for (int i = 0; i < causalAgents.Length; i++)
            {
                if (causalAgents[i] is T tEntity)
                {
                    tEntities.Add(tEntity);
                }
            }
            return tEntities.ToArray();
        }

        public virtual T[] CausalActions<T>() where T : IAction
        {
            List<T> tActions = new List<T>();
            for (int i = 0; i < causalActions.Length; i++)
            {
                if (causalActions[i] is T tAction)
                {
                    tActions.Add(tAction);
                }
            }
            return tActions.ToArray();
        }

        public virtual T[] CausalPieces<T>() where T : IThing
        {
            List<T> tElements = new List<T>();
            for (int i = 0; i < causalPieces.Length; i++)
            {
                if (causalPieces[i] is T tElement)
                {
                    tElements.Add(tElement);
                }
            }
            return tElements.ToArray();
        }

        public virtual T[] EffectedAgents<T>() where T : IAgent
        {
            List<T> tEntities = new List<T>();
            for (int i = 0; i < effectedAgents.Length; i++)
            {
                if (effectedAgents[i] is T tEntity)
                {
                    tEntities.Add(tEntity);
                }
            }
            return tEntities.ToArray();
        }

        public virtual T[] EffectedPieces<T>() where T : IThing
        {
            List<T> tElements = new List<T>();
            for (int i = 0; i < effectedPieces.Length; i++)
            {
                if (effectedPieces[i] is T tElement)
                {
                    tElements.Add(tElement);
                }
            }
            return tElements.ToArray();
        }

        public virtual void EndInteraction()
        {
            OnEndInteraction?.Invoke();
        }

        public virtual T[] MotivateReactions<T>() where T:IAction
        {
            if (typeof(T).IsAssignableFrom(typeof(UnityGameAction)))
            {
                List<UnityGameAction> movtivatedActions = new List<UnityGameAction>();

                for(int i =0; i < interactionEffects.Count; i++)
                {

                }
            }

            return new T[0];
        }
    }
}