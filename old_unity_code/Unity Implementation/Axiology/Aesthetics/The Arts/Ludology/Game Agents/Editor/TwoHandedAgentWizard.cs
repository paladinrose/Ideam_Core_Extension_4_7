
using UnityEditor;

using UnityEngine;
using UnityEngine.Animations.Rigging;

using Ideam.Unity.Metaphysics.Mind;
using Ideam.Unity.Science.Natural.Physics;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    class TwoHandedAgentWizard : ScriptableWizard
    {
        public Animator agentRoot;
        public Transform leftHandRoot, rightHandRoot;

        public bool createAnimationRigging = true;
        public bool createHandIK, createFingerControls;
        public bool createActiveRagdoll = true;
        public bool createAsGhost;

        [MenuItem("Tools/Ideam/Ludology/Two Handed Agent", false, 2000)]
        static void CreateWizard()
        {
            DisplayWizard<TwoHandedAgentWizard>("Create Two Handed Agent");
        }

        private void OnWizardUpdate()
        {
            if (!agentRoot) { errorString = "Plesae set a root agent transform to continue."; }
            else { errorString = ""; }
        }

        private void OnWizardCreate()
        {
            GameObject agentObject = new GameObject(agentRoot.name);
            agentObject.transform.position = agentRoot.transform.position;
            agentObject.transform.eulerAngles = agentRoot.transform.eulerAngles;

            agentRoot.transform.SetParent(agentObject.transform, true);

            TwoHandedAgent agent = agentObject.AddComponent<TwoHandedAgent>();
            agent.agentAnimator = agentRoot;

            UnityMind agentMind = agentObject.AddComponent<UnityMind>();
            agent.mind = agentMind;

            if (createActiveRagdoll)
            {

                Rigidbody agentBody = agentObject.AddComponent<Rigidbody>();
                agent.agentBody = agentBody;

                //NEEDS UPDATED TO PHYSICALANIMATOR
               // ActiveRagdoll ragdoll = ActiveRagdoll.CreateRagdoll(agentRoot, new Transform[] { leftHandRoot, rightHandRoot });


            }

            if (createAnimationRigging)
            {
                HandAnimationRigging rig =HandAnimationRigging.CreateHandAnimationRigging(agentRoot, new Transform[] { leftHandRoot, rightHandRoot });
               
                agent.handsRig = rig;
            }
        }

    }
}