
using UnityEditor;

using UnityEngine;
using UnityEngine.Animations.Rigging;

using Ideam.Unity.Metaphysics.Mind;
using Ideam.Unity.Science.Natural.Physics;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    class HumanoidAgentWizard : ScriptableWizard
    {
        public Animator animator = null;
        public bool createAnimationRigging = true;
        public bool createActiveRagdoll = true;

        public Transform[] ragdollRoots;

        [MenuItem("Tools/Ideam/Ludology/Humanoid Agent", false, 2000)]
        static void CreateWizard()
        {
            DisplayWizard<HumanoidAgentWizard>("Create Humanoid Agent");
        }

        private void OnWizardUpdate()
        {
            if (!animator) { errorString = "Plesae set a humanoid-avatar animator to continue."; }
            else { errorString = ""; }
        }

        private void OnWizardCreate()
        {
            Transform oldParent = animator.transform.parent;
            
            GameObject agentObject = new GameObject(animator.name);
            agentObject.transform.SetParent(oldParent);
            agentObject.transform.position = animator.transform.position;
            agentObject.transform.eulerAngles = animator.transform.eulerAngles;
            
            animator.transform.SetParent(agentObject.transform, true);


            HumanoidAgent agent = agentObject.AddComponent<HumanoidAgent>();
            agent.agentAnimator = animator;

            UnityMind agentMind = agentObject.AddComponent<UnityMind>();
            agent.mind = agentMind;
            /*

            6. If a character has eyes, get the point right between them, then measure down to the bottoms of the feet. Set this as Humandoid Agent's Eyeline Height. 
            */
            if (createActiveRagdoll)
            {
                Rigidbody agentBody = agentObject.AddComponent<Rigidbody>();
                agent.agentBody = agentBody;
                
                //NEEDS UPGRADED TO PHYSICALANIMATOR
               // ActiveRagdoll ragdoll = ActiveRagdoll.CreateRagdoll(animator, ragdollRoots);

                //ragdoll.ChangePartJoint(0, RagdollJointType.Ball);
                //ragdoll.parts[0].joint.connectedBody = agentBody;

            }

            if (createAnimationRigging)
            {
                HumanoidAnimationRigging rig = HumanoidAnimationRigging.CreateRiggingFromAnimator(animator, new Transform[0]);

                agent.humanoidRig = rig;
            }
        }

    }
}