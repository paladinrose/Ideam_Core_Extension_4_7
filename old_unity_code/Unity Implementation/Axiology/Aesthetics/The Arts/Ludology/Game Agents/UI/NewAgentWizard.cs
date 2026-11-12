using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Unity.UI;
namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    public class NewAgentWizard : UIWizard
    {
        public UnityGame game;
        public Animator animator;
        public bool isPhysical, createRigidbody, createCapsuleCollider, createGhostAgent;
        
        protected override IEnumerator UpdateWizard()
        {
            GameObject agentObject = new GameObject();
            Transform agentRoot = null;
            Rigidbody rigidbody = null;
            Collider collider = null;
            if (animator)
            {
                agentObject.transform.SetParent(animator.transform); 
                agentRoot = animator.transform;
                rigidbody = animator.gameObject.GetComponent<Rigidbody>();
                collider = animator.gameObject.GetComponent<Collider>();

            }
            else { agentRoot = agentObject.transform; }

            if (isPhysical)
            {
                if (createRigidbody && rigidbody == null)
                {
                    rigidbody = agentRoot.gameObject.AddComponent<Rigidbody>();
                }

                if(collider == null && createCapsuleCollider)
                {
                    collider = agentRoot.gameObject.AddComponent<CapsuleCollider>();
                }
            }

            UnityGameAgent agent = null;
            
            if (createGhostAgent)
            {
                agent = GhostAgent.AddGhostAgent(agentObject, animator, rigidbody, agentRoot, game, true);
            }
            else
            {
                agent = UnityGameAgent.AddUnityGameAgent(agentObject, animator, rigidbody, agentRoot, game, true);
            }

            yield break;   
        }
    }

    public class NewAgentWizardUI : UIWizardUI<NewAgentWizard>
    {

    }
}