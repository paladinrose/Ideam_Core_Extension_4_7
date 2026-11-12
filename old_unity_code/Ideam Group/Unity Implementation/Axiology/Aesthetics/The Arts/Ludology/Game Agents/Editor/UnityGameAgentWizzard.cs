
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

using Ideam.Unity.UI;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class UnityGameAgentWizzard : EditorWindow
    {
        public UnityGame game = null;
        public Animator animator = null;
        public enum AnimationRiggingType { Generic, FreeHand, IKHand, Humanoid };

        public AnimationRiggingType riggingType;
        public AnimationRiggingProfile animationRiggingProfile = null;

        public bool createObjectForAgentMindAndSenses, createRigidbody = true, createAIController;

        protected NewAgentWizard newAgentWizard;

        [MenuItem("Tools/Ideam/Ludology/Unity Game Agent Wizzard", false, 2000)]
        static void CreateWizard()
        {
            UnityGameAgentWizzard wiz = GetWindow<UnityGameAgentWizzard>();
        }


        private void OnWizardCreate()
        {
            Transform parent = null; Rigidbody agentBody = null;
            if(animator != null) { parent = animator.transform.parent; agentBody = animator.GetComponent<Rigidbody>(); }
            
            GameObject agentObject = new GameObject("Game Agent");
            if (animator != null) { agentObject.name = animator.gameObject.name + " (Game Agent)"; }

            if (parent != null)
            {
                agentObject.transform.SetParent(parent, false);
                agentObject.transform.localEulerAngles = agentObject.transform.localPosition = Vector3.zero;

            }

            if (createRigidbody)
            {
                agentBody = agentObject.AddComponent<Rigidbody>();
            }

            UnityGameAgent gameAgent = UnityGameAgent.AddUnityGameAgent(agentObject, animator, agentBody, parent, game, createObjectForAgentMindAndSenses);
            if(parent == null) { gameAgent.AgentRoot = gameAgent.transform; }

            if (animator != null)
            {
                gameAgent.transform.position = animator.transform.position;
                gameAgent.transform.eulerAngles = animator.transform.eulerAngles;
                animator.transform.SetParent(gameAgent.transform, false);
                animator.transform.localEulerAngles = animator.transform.localPosition = Vector3.zero;

                switch (riggingType)
                {
                    case AnimationRiggingType.Generic:
                        break;
                    case AnimationRiggingType.FreeHand:
                        break;
                    case AnimationRiggingType.IKHand:
                        break;
                    case AnimationRiggingType.Humanoid:
                        //HumanoidAnimationRigging.CreateRiggingFromAnimator(animator, new Transform[0]);
                        break;
                }
            }


            if (createAIController)
            {
                UnityAIController aiController = UnityAIController.AddUnityAIController(gameAgent.gameObject, agentBody);
                aiController.controlledMind = gameAgent.mind;
                aiController.agent = gameAgent;
            }

            //Get the UnityGameActions from the gameAgent (this is our Defaults list)
            UnityGameAction[] defaultActions = gameAgent.actions;
            //Attach them and their events to Move, Turn, and Push Controls.


            EditorUtility.SetDirty(agentObject);
            if(game != null) 
            {
               AssetDatabase.SaveAssets();
            }
        }


    }
}
